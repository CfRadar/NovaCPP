#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <map>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <iostream>
#include <sstream>
#include "sqlite.hpp"

namespace np {

// Metadata capturing checkpoint lineage, node origin, and timestamps
struct CheckpointMetadata {
    std::string threadId;
    int64_t checkpointId = 0;
    int64_t parentId = 0;
    std::string nodeName;
    int64_t createdAt = 0;
    std::string metadata = "{}";
};

// Abstract interface for durable state checkpointing
template <typename TState>
class ICheckpointer {
public:
    virtual ~ICheckpointer() = default;

    virtual void save(
        const std::string& threadId,
        int64_t checkpointId,
        int64_t parentId,
        const std::string& nodeName,
        const TState& state,
        const std::string& metadata = "{}"
    ) = 0;

    virtual bool load(
        const std::string& threadId,
        int64_t checkpointId,
        TState& outState
    ) = 0;

    virtual bool loadLatest(
        const std::string& threadId,
        TState& outState,
        CheckpointMetadata& outMeta
    ) = 0;

    virtual std::vector<CheckpointMetadata> getHistory(
        const std::string& threadId
    ) = 0;

    virtual void forkTimeline(
        const std::string& sourceThread,
        int64_t sourceCheckpoint,
        const std::string& newThread
    ) = 0;

    virtual bool deleteThread(
        const std::string& threadId
    ) = 0;
};

// High-speed in-memory checkpointer for ephemeral sessions, sub-microsecond time-travel, and testing
template <typename TState>
class InMemoryCheckpointer : public ICheckpointer<TState> {
private:
    struct Entry {
        CheckpointMetadata meta;
        TState state;
    };
    mutable std::mutex m_mtx;
    std::unordered_map<std::string, std::vector<Entry>> m_threads;

public:
    void save(
        const std::string& threadId,
        int64_t checkpointId,
        int64_t parentId,
        const std::string& nodeName,
        const TState& state,
        const std::string& metadata = "{}"
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        CheckpointMetadata meta{threadId, checkpointId, parentId, nodeName, now, metadata};
        auto& vec = m_threads[threadId];
        
        bool replaced = false;
        if (!vec.empty() && vec.back().meta.checkpointId == checkpointId) {
            vec.back() = Entry{meta, state};
            replaced = true;
        } else {
            for (auto& item : vec) {
                if (item.meta.checkpointId == checkpointId) {
                    item = Entry{meta, state};
                    replaced = true;
                    break;
                }
            }
        }
        if (!replaced) {
            vec.push_back(Entry{meta, state});
        }
    }

    bool load(
        const std::string& threadId,
        int64_t checkpointId,
        TState& outState
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_threads.find(threadId);
        if (it == m_threads.end()) return false;

        const auto& vec = it->second;
        if (checkpointId >= 0 && static_cast<size_t>(checkpointId) < vec.size() &&
            vec[static_cast<size_t>(checkpointId)].meta.checkpointId == checkpointId) {
            outState = vec[static_cast<size_t>(checkpointId)].state;
            return true;
        }

        for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
            if (rit->meta.checkpointId == checkpointId) {
                outState = rit->state;
                return true;
            }
        }
        return false;
    }

    bool loadLatest(
        const std::string& threadId,
        TState& outState,
        CheckpointMetadata& outMeta
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_threads.find(threadId);
        if (it != m_threads.end() && !it->second.empty()) {
            outState = it->second.back().state;
            outMeta = it->second.back().meta;
            return true;
        }
        return false;
    }

    std::vector<CheckpointMetadata> getHistory(
        const std::string& threadId
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::vector<CheckpointMetadata> history;
        auto it = m_threads.find(threadId);
        if (it != m_threads.end()) {
            history.reserve(it->second.size());
            for (const auto& entry : it->second) {
                history.push_back(entry.meta);
            }
        }
        return history;
    }

    void forkTimeline(
        const std::string& sourceThread,
        int64_t sourceCheckpoint,
        const std::string& newThread
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_threads.find(sourceThread);
        if (it == m_threads.end()) return;

        std::vector<Entry> cloned;
        for (const auto& entry : it->second) {
            if (entry.meta.checkpointId <= sourceCheckpoint) {
                Entry cp = entry;
                cp.meta.threadId = newThread;
                cloned.push_back(std::move(cp));
            }
        }
        m_threads[newThread] = std::move(cloned);
    }

    bool deleteThread(const std::string& threadId) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_threads.erase(threadId) > 0;
    }
};

// Embedded SQLite WAL Checkpointer with binary BLOB serialization and microsecond persistence
template <typename TState>
class SQLiteCheckpointer : public ICheckpointer<TState> {
public:
    using Serializer = std::function<std::string(const TState&)>;
    using Deserializer = std::function<bool(const std::string&, TState&)>;

private:
    std::shared_ptr<SQLiteConnection> m_conn;
    Serializer m_serializer;
    Deserializer m_deserializer;
    mutable std::mutex m_mtx;

public:
    explicit SQLiteCheckpointer(
        std::shared_ptr<SQLiteConnection> conn,
        Serializer serializer = nullptr,
        Deserializer deserializer = nullptr
    ) : m_conn(std::move(conn)),
        m_serializer(std::move(serializer)),
        m_deserializer(std::move(deserializer))
    {
        if (!m_serializer) {
            m_serializer = [](const TState& state) -> std::string {
                return std::string(reinterpret_cast<const char*>(&state), sizeof(TState));
            };
        }
        if (!m_deserializer) {
            m_deserializer = [](const std::string& blob, TState& outState) -> bool {
                if (blob.size() == sizeof(TState)) {
                    std::memcpy(&outState, blob.data(), sizeof(TState));
                    return true;
                }
                return false;
            };
        }
        initializeTables();
    }

    explicit SQLiteCheckpointer(
        const std::string& dbPath = ":memory:",
        Serializer serializer = nullptr,
        Deserializer deserializer = nullptr
    ) : SQLiteCheckpointer(std::make_shared<SQLiteConnection>(dbPath), serializer, deserializer) {}

    void initializeTables() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_conn->execute(
            "CREATE TABLE IF NOT EXISTS nova_checkpoints ("
            "  thread_id TEXT NOT NULL,"
            "  checkpoint_id INTEGER NOT NULL,"
            "  parent_id INTEGER,"
            "  node_name TEXT NOT NULL,"
            "  created_at INTEGER NOT NULL,"
            "  state_blob BLOB NOT NULL,"
            "  metadata TEXT,"
            "  PRIMARY KEY (thread_id, checkpoint_id)"
            ");"
        );
        m_conn->execute(
            "CREATE INDEX IF NOT EXISTS idx_thread_ckpt ON nova_checkpoints(thread_id, checkpoint_id DESC);"
        );
    }

    void save(
        const std::string& threadId,
        int64_t checkpointId,
        int64_t parentId,
        const std::string& nodeName,
        const TState& state,
        const std::string& metadata = "{}"
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        std::string blob = m_serializer(state);

        m_conn->execute(
            "INSERT OR REPLACE INTO nova_checkpoints (thread_id, checkpoint_id, parent_id, node_name, created_at, state_blob, metadata) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);",
            threadId, checkpointId, parentId, nodeName, now, SQLiteBlob{blob}, metadata
        );
    }

    bool load(
        const std::string& threadId,
        int64_t checkpointId,
        TState& outState
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto rows = m_conn->queryAll(
            "SELECT state_blob FROM nova_checkpoints WHERE thread_id = ? AND checkpoint_id = ?;",
            threadId, checkpointId
        );
        if (rows.empty()) return false;
        return m_deserializer(rows[0].at("state_blob"), outState);
    }

    bool loadLatest(
        const std::string& threadId,
        TState& outState,
        CheckpointMetadata& outMeta
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto rows = m_conn->queryAll(
            "SELECT checkpoint_id, parent_id, node_name, created_at, state_blob, metadata "
            "FROM nova_checkpoints WHERE thread_id = ? ORDER BY checkpoint_id DESC LIMIT 1;",
            threadId
        );
        if (rows.empty()) return false;

        const auto& r = rows[0];
        outMeta.threadId = threadId;
        outMeta.checkpointId = std::stoll(r.at("checkpoint_id"));
        outMeta.parentId = std::stoll(r.at("parent_id"));
        outMeta.nodeName = r.at("node_name");
        outMeta.createdAt = std::stoll(r.at("created_at"));
        outMeta.metadata = r.at("metadata");

        return m_deserializer(r.at("state_blob"), outState);
    }

    std::vector<CheckpointMetadata> getHistory(
        const std::string& threadId
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto rows = m_conn->queryAll(
            "SELECT checkpoint_id, parent_id, node_name, created_at, metadata "
            "FROM nova_checkpoints WHERE thread_id = ? ORDER BY checkpoint_id ASC;",
            threadId
        );
        std::vector<CheckpointMetadata> history;
        history.reserve(rows.size());
        for (const auto& r : rows) {
            CheckpointMetadata meta;
            meta.threadId = threadId;
            meta.checkpointId = std::stoll(r.at("checkpoint_id"));
            meta.parentId = std::stoll(r.at("parent_id"));
            meta.nodeName = r.at("node_name");
            meta.createdAt = std::stoll(r.at("created_at"));
            meta.metadata = r.at("metadata");
            history.push_back(meta);
        }
        return history;
    }

    void forkTimeline(
        const std::string& sourceThread,
        int64_t sourceCheckpoint,
        const std::string& newThread
    ) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_conn->execute(
            "INSERT INTO nova_checkpoints (thread_id, checkpoint_id, parent_id, node_name, created_at, state_blob, metadata) "
            "SELECT ?, checkpoint_id, parent_id, node_name, created_at, state_blob, metadata "
            "FROM nova_checkpoints WHERE thread_id = ? AND checkpoint_id <= ?;",
            newThread, sourceThread, sourceCheckpoint
        );
    }

    bool deleteThread(const std::string& threadId) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_conn->execute("DELETE FROM nova_checkpoints WHERE thread_id = ?;", threadId);
        return true;
    }
};

} // namespace np

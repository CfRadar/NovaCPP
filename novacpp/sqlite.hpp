#pragma once

#include "sqlite/sqlite3.h"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <mutex>
#include <iostream>
#include <filesystem>
#include <type_traits>
#include <cstdint>

namespace np {

class SQLiteConnection;

namespace detail {
    inline void bindValue(sqlite3_stmt* stmt, int index, int val) {
        sqlite3_bind_int(stmt, index, val);
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, long val) {
        sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, long long val) {
        sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, unsigned int val) {
        sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, unsigned long val) {
        sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, unsigned long long val) {
        sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, double val) {
        sqlite3_bind_double(stmt, index, val);
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, float val) {
        sqlite3_bind_double(stmt, index, static_cast<double>(val));
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, bool val) {
        sqlite3_bind_int(stmt, index, val ? 1 : 0);
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, const std::string& val) {
        sqlite3_bind_text(stmt, index, val.c_str(), -1, SQLITE_TRANSIENT);
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, const char* val) {
        if (val) {
            sqlite3_bind_text(stmt, index, val, -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, index);
        }
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, std::string_view val) {
        sqlite3_bind_text(stmt, index, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
    }
    inline void bindValue(sqlite3_stmt* stmt, int index, std::nullptr_t) {
        sqlite3_bind_null(stmt, index);
    }
} // namespace detail

// RAII Transaction scope guard
class Transaction {
private:
    SQLiteConnection& m_conn;
    bool m_committed = false;
    bool m_rolledBack = false;

public:
    explicit Transaction(SQLiteConnection& conn);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit();
    void rollback();
    bool isCommitted() const { return m_committed; }
};

class SQLiteConnection {
private:
    struct CachedStatement {
        sqlite3_stmt* stmt = nullptr;
        bool in_use = false;
    };

    sqlite3* m_db = nullptr;
    std::string m_path;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, CachedStatement> m_stmtCache;

    sqlite3_stmt* acquireStatement(const std::string& sql, bool& isCached) {
        auto it = m_stmtCache.find(sql);
        if (it != m_stmtCache.end() && !it->second.in_use) {
            it->second.in_use = true;
            isCached = true;
            sqlite3_reset(it->second.stmt);
            sqlite3_clear_bindings(it->second.stmt);
            return it->second.stmt;
        }

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("[NovaCPP SQLite Prepare Error] " + std::string(sqlite3_errmsg(m_db)) + " (SQL: " + sql + ")");
        }

        if (it == m_stmtCache.end() && m_stmtCache.size() < 128) {
            m_stmtCache[sql] = {stmt, true};
            isCached = true;
        } else {
            isCached = false;
        }
        return stmt;
    }

    void releaseStatement(const std::string& sql, sqlite3_stmt* stmt, bool isCached) {
        if (isCached) {
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
            auto it = m_stmtCache.find(sql);
            if (it != m_stmtCache.end()) {
                it->second.in_use = false;
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }

    void clearStatementCache() {
        for (auto& pair : m_stmtCache) {
            if (pair.second.stmt) {
                sqlite3_finalize(pair.second.stmt);
                pair.second.stmt = nullptr;
            }
        }
        m_stmtCache.clear();
    }

public:
    explicit SQLiteConnection(const std::string& dbPath) : m_path(dbPath) {
        // Automatically ensure parent directory exists for file-backed databases
        try {
            if (dbPath != ":memory:" && dbPath.find("mode=memory") == std::string::npos) {
                std::filesystem::path p(dbPath);
                if (p.has_parent_path()) {
                    std::filesystem::create_directories(p.parent_path());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[NovaCPP SQLite] Warning: could not create parent directories: " << e.what() << std::endl;
        }

        int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI;
        if (sqlite3_open_v2(dbPath.c_str(), &m_db, flags, nullptr) != SQLITE_OK) {
            std::string err = m_db ? sqlite3_errmsg(m_db) : "Unknown error";
            if (m_db) {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
            throw std::runtime_error("[NovaCPP SQLite] Failed to open database at '" + dbPath + "': " + err);
        }

        // Apply production WAL Mode & high-concurrency pragmas automatically for persistent DBs
        if (dbPath != ":memory:" && dbPath.find("mode=memory") == std::string::npos) {
            execute("PRAGMA journal_mode = WAL;");
            execute("PRAGMA synchronous = NORMAL;");
            execute("PRAGMA cache_size = -64000;"); // ~64MB memory page cache
            execute("PRAGMA busy_timeout = 5000;"); // 5s timeout before SQLITE_BUSY
            execute("PRAGMA foreign_keys = ON;");
        }
    }

    ~SQLiteConnection() {
        close();
    }

    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    SQLiteConnection(SQLiteConnection&& other) noexcept {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_db = other.m_db;
        m_path = std::move(other.m_path);
        m_stmtCache = std::move(other.m_stmtCache);
        other.m_db = nullptr;
        other.m_stmtCache.clear();
    }

    SQLiteConnection& operator=(SQLiteConnection&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            close();
            m_db = other.m_db;
            m_path = std::move(other.m_path);
            m_stmtCache = std::move(other.m_stmtCache);
            other.m_db = nullptr;
            other.m_stmtCache.clear();
        }
        return *this;
    }

    void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
        clearStatementCache();
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    const std::string& getPath() const {
        return m_path;
    }

    sqlite3* getRawHandle() {
        return m_db;
    }

    // Execute raw SQL script without parameter binding (supports multi-statement DDL)
    void execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db) throw std::runtime_error("[NovaCPP SQLite] Database connection is closed.");

        char* errMsg = nullptr;
        if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string err = errMsg ? errMsg : "Unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("[NovaCPP SQLite Exec Error] " + err + " (SQL: " + sql + ")");
        }
    }

    // Execute prepared statement with typed parameter binding
    template <typename First, typename... Rest>
    void execute(const std::string& sql, First&& first, Rest&&... rest) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db) throw std::runtime_error("[NovaCPP SQLite] Database connection is closed.");

        bool isCached = false;
        sqlite3_stmt* stmt = acquireStatement(sql, isCached);
        try {
            int idx = 1;
            detail::bindValue(stmt, idx++, std::forward<First>(first));
            (detail::bindValue(stmt, idx++, std::forward<Rest>(rest)), ...);

            int rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
                std::string err = sqlite3_errmsg(m_db);
                releaseStatement(sql, stmt, isCached);
                throw std::runtime_error("[NovaCPP SQLite Exec Error] " + err + " (SQL: " + sql + ")");
            }
            releaseStatement(sql, stmt, isCached);
        } catch (...) {
            releaseStatement(sql, stmt, isCached);
            throw;
        }
    }

    // Execute SELECT query with row callback and optional bound parameters
    template <typename... Args>
    void query(const std::string& sql, 
               std::function<void(const std::map<std::string, std::string>&)> rowCallback,
               Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db) throw std::runtime_error("[NovaCPP SQLite] Database connection is closed.");

        bool isCached = false;
        sqlite3_stmt* stmt = acquireStatement(sql, isCached);
        try {
            if constexpr (sizeof...(Args) > 0) {
                int idx = 1;
                (detail::bindValue(stmt, idx++, std::forward<Args>(args)), ...);
            }

            int colCount = sqlite3_column_count(stmt);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::map<std::string, std::string> row;
                for (int i = 0; i < colCount; ++i) {
                    const char* colName = sqlite3_column_name(stmt, i);
                    const unsigned char* val = sqlite3_column_text(stmt, i);
                    row[colName ? colName : ""] = val ? reinterpret_cast<const char*>(val) : "";
                }
                rowCallback(row);
            }
            releaseStatement(sql, stmt, isCached);
        } catch (...) {
            releaseStatement(sql, stmt, isCached);
            throw;
        }
    }

    // Return all rows as a vector of maps (with optional parameter binding)
    template <typename... Args>
    std::vector<std::map<std::string, std::string>> queryAll(const std::string& sql, Args&&... args) {
        std::vector<std::map<std::string, std::string>> results;
        query(sql, [&](const std::map<std::string, std::string>& row) {
            results.push_back(row);
        }, std::forward<Args>(args)...);
        return results;
    }

    // Return a single scalar string result (e.g. SELECT count(*))
    template <typename... Args>
    std::string queryScalar(const std::string& sql, Args&&... args) {
        if constexpr (sizeof...(Args) == 0) {
            std::string result = "";
            bool found = false;
            query(sql, [&](const std::map<std::string, std::string>& row) {
                if (!found && !row.empty()) {
                    result = row.begin()->second;
                    found = true;
                }
            });
            return result;
        } else if constexpr (sizeof...(Args) == 1) {
            if (sql.find('?') == std::string::npos && sql.find(':') == std::string::npos) {
                std::string fallback = "";
                auto getFallback = [&](auto&& v) {
                    if constexpr (std::is_convertible_v<decltype(v), std::string>) {
                        fallback = std::string(v);
                    }
                };
                (getFallback(args), ...);

                std::string result = fallback;
                bool found = false;
                query(sql, [&](const std::map<std::string, std::string>& row) {
                    if (!found && !row.empty()) {
                        result = row.begin()->second;
                        found = true;
                    }
                });
                return result;
            } else {
                std::string result = "";
                bool found = false;
                query(sql, [&](const std::map<std::string, std::string>& row) {
                    if (!found && !row.empty()) {
                        result = row.begin()->second;
                        found = true;
                    }
                }, std::forward<Args>(args)...);
                return result;
            }
        } else {
            std::string result = "";
            bool found = false;
            query(sql, [&](const std::map<std::string, std::string>& row) {
                if (!found && !row.empty()) {
                    result = row.begin()->second;
                    found = true;
                }
            }, std::forward<Args>(args)...);
            return result;
        }
    }

    // Execute callback inside an atomic transaction on this connection
    template <typename Callback>
    auto transaction(Callback&& fn) -> typename std::enable_if_t<std::is_void_v<std::invoke_result_t<Callback, SQLiteConnection&>>, void> {
        Transaction tx(*this);
        fn(*this);
        tx.commit();
    }

    template <typename Callback>
    auto transaction(Callback&& fn) -> typename std::enable_if_t<!std::is_void_v<std::invoke_result_t<Callback, SQLiteConnection&>>, std::invoke_result_t<Callback, SQLiteConnection&>> {
        Transaction tx(*this);
        auto res = fn(*this);
        tx.commit();
        return res;
    }
};

inline Transaction::Transaction(SQLiteConnection& conn) : m_conn(conn) {
    m_conn.execute("BEGIN TRANSACTION;");
}

inline Transaction::~Transaction() {
    if (!m_committed && !m_rolledBack) {
        try {
            m_conn.execute("ROLLBACK;");
        } catch (...) {}
    }
}

inline void Transaction::commit() {
    if (!m_committed && !m_rolledBack) {
        m_conn.execute("COMMIT;");
        m_committed = true;
    }
}

inline void Transaction::rollback() {
    if (!m_committed && !m_rolledBack) {
        m_conn.execute("ROLLBACK;");
        m_rolledBack = true;
    }
}

} // namespace np

#pragma once

#include "sqlite/sqlite3.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdexcept>
#include <mutex>
#include <iostream>
#include <filesystem>

namespace np {

class SQLiteConnection {
private:
    sqlite3* m_db = nullptr;
    std::string m_path;
    mutable std::mutex m_mutex;

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
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_db = other.m_db;
        m_path = std::move(other.m_path);
        other.m_db = nullptr;
    }

    SQLiteConnection& operator=(SQLiteConnection&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            close();
            m_db = other.m_db;
            m_path = std::move(other.m_path);
            other.m_db = nullptr;
        }
        return *this;
    }

    void close() {
        std::lock_guard<std::mutex> lock(m_mutex);
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

    // Execute DDL or mutations (CREATE, INSERT, UPDATE, DELETE)
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

    // Execute SELECT query with row callback
    void query(const std::string& sql, 
               std::function<void(const std::map<std::string, std::string>&)> rowCallback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db) throw std::runtime_error("[NovaCPP SQLite] Database connection is closed.");

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("[NovaCPP SQLite Prepare Error] " + std::string(sqlite3_errmsg(m_db)) + " (SQL: " + sql + ")");
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
        sqlite3_finalize(stmt);
    }

    // Return all rows as a vector of maps
    std::vector<std::map<std::string, std::string>> queryAll(const std::string& sql) {
        std::vector<std::map<std::string, std::string>> results;
        query(sql, [&](const std::map<std::string, std::string>& row) {
            results.push_back(row);
        });
        return results;
    }

    // Return a single scalar string result (e.g. SELECT count(*))
    std::string queryScalar(const std::string& sql, const std::string& fallback = "") {
        std::string result = fallback;
        bool found = false;
        query(sql, [&](const std::map<std::string, std::string>& row) {
            if (!found && !row.empty()) {
                result = row.begin()->second;
                found = true;
            }
        });
        return result;
    }
};

} // namespace np


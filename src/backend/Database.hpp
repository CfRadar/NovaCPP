#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include "../../novacpp/sqlite.hpp"
#include "../../novacpp/db_pool.hpp"

class Database {
private:
    static std::unique_ptr<np::ConnectionPool<np::SQLiteConnection>> s_pool;
    static std::unique_ptr<np::SQLiteConnection> s_primaryInstance;
    static std::string s_activePath;
    static size_t s_poolSize;
    static std::mutex s_dbMutex;

public:
    // Connect to database and initialize connection pool (default pool size: 10)
    static void connect(const std::string& connectionString = "", size_t poolSize = 10);

    // Acquire an RAII Connection Guard with bounded timeout (default: 1500ms)
    static np::ConnectionPool<np::SQLiteConnection>::Guard acquire(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1500)
    );

    // Execute DDL or mutations (CREATE TABLE, INSERT, UPDATE, DELETE) via pooled connection
    static void execute(const std::string& sql);

    // Backward-compatible query execution method
    static void executeQuery(const std::string& query);

    // Execute SELECT query and return all rows via pooled connection
    static std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    // Execute query and return first scalar value
    static std::string queryScalar(const std::string& sql, const std::string& fallback = "");

    // Returns true if an active database pool is initialized
    static bool isConnected();

    // Returns count of currently available/idle connections in pool
    static size_t getAvailableConnections();

    // Returns total pool capacity
    static size_t getPoolCapacity();

    // Direct access to primary instance or pool
    static np::SQLiteConnection& getInstance();
    static np::ConnectionPool<np::SQLiteConnection>& getPool();
};

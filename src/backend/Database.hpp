#pragma once

/* ============================================================================
   NOVACPP DATABASE SERVICE (src/backend/Database.hpp)
   SQLite WAL Connection Pool, Prepared Statements & Scoped Transactions.

   BOILERPLATE USAGE:
   1. Connect at startup:
      Database::connect(); // reads DATABASE_URL from .env or defaults to ./data/nova.db

   2. Execute DDL / Mutations (supports typed parameter binding):
      Database::execute("CREATE TABLE IF NOT EXISTS items (id INTEGER PRIMARY KEY, name TEXT);");
      Database::execute("INSERT INTO items (name) VALUES (?);", "Widget");

   3. Query Rows or Scalars (with parameters):
      auto rows = Database::query("SELECT id, name FROM items WHERE name = ?;", "Widget");
      std::string count = Database::queryScalar("SELECT COUNT(*) FROM items;");

   4. Scoped Atomic ACID Transactions:
      Database::transaction([](np::SQLiteConnection& db) {
          db.execute("UPDATE accounts SET balance = balance - ? WHERE id = ?;", 50, senderId);
          db.execute("UPDATE accounts SET balance = balance + ? WHERE id = ?;", 50, receiverId);
      }); // Automatically commits on success; rolls back on any exception

   5. Scoped RAII Connection Guard:
      {
          auto guard = Database::acquire();
          guard->execute("INSERT INTO items (name) VALUES (?);", "Gadget");
      } // Connection handle returned automatically to pool on scope exit
   ============================================================================ */

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <type_traits>
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

    // Execute raw DDL or SQL scripts without parameters via pooled connection
    static void execute(const std::string& sql);

    // Execute prepared statement with typed parameter binding via pooled connection
    template <typename First, typename... Rest>
    static void execute(const std::string& sql, First&& first, Rest&&... rest) {
        auto conn = acquire();
        conn->execute(sql, std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    // Backward-compatible query execution method
    static void executeQuery(const std::string& query);

    // Execute SELECT query and return all rows via pooled connection
    static std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    // Execute SELECT query with bound parameters and return all rows
    template <typename First, typename... Rest>
    static std::vector<std::map<std::string, std::string>> query(const std::string& sql, First&& first, Rest&&... rest) {
        auto conn = acquire();
        return conn->queryAll(sql, std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    // Execute query and return first scalar value (supports optional fallback)
    static std::string queryScalar(const std::string& sql, const std::string& fallback = "");

    // Execute query with bound parameters and return first scalar value
    template <typename First, typename... Rest>
    static std::string queryScalar(const std::string& sql, First&& first, Rest&&... rest) {
        auto conn = acquire();
        return conn->queryScalar(sql, std::forward<First>(first), std::forward<Rest>(rest)...);
    }

    // Execute a series of operations atomically inside a scoped transaction on a single pooled connection
    template <typename Callback>
    static auto transaction(Callback&& fn) -> std::invoke_result_t<Callback, np::SQLiteConnection&> {
        auto guard = acquire();
        return guard->transaction(std::forward<Callback>(fn));
    }

    // Returns true if an active database pool is initialized
    static bool isConnected();

    // Closes and releases all connections in the pool
    static void close();

    // Returns count of currently available/idle connections in pool
    static size_t getAvailableConnections();

    // Returns total pool capacity
    static size_t getPoolCapacity();

    // Direct access to primary instance or pool
    static np::SQLiteConnection& getInstance();
    static np::ConnectionPool<np::SQLiteConnection>& getPool();
};

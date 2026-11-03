---
name: nova-database
description: >-
  Manage embedded SQLite 3.49.1 WAL persistence, connection pooling, and parameterized queries in NovaCPP.
  Use when the user asks to create database tables, execute SQL queries, use connection pools, run transactions,
  or perform CRUD persistence in a Nova project. Not for session state (see nova-sessions-state).
license: MIT
compatibility: Requires a NovaCPP project with C++17 compiler (MSVC 2022, GCC 11+, Clang 13+).
metadata:
  nova-version: ">=2.1.0"
  author: novacpp-core-team
  version: "1.0.0"
---

# NovaCPP SQLite WAL Database Engine & Connection Pooling

<!-- symbols: Database, SQLiteConnection, ConnectionPool, acquire -->

## When to use
Use this skill when defining database tables, executing SELECT/INSERT/UPDATE/DELETE queries, managing atomic transactions, or configuring thread-safe SQLite connection pooling in `src/backend/Database.cpp`.

## Prerequisites
- A NovaCPP project with `src/backend/Database.hpp` and `src/backend/Database.cpp`.
- Embedded SQLite amalgamation (`novacpp/sqlite/sqlite3.c`).

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include "src/backend/Database.hpp"

void demonstrateDatabaseUsage() {
    Database::connect();

    // 1. Parameterized DDL / Write
    Database::execute("CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, title TEXT, body TEXT);");
    Database::execute("INSERT INTO notes (title, body) VALUES (?, ?);", "Meeting Notes", "Discuss Q4 roadmap");

    // 2. Parameterized Query
    auto rows = Database::query("SELECT id, title, body FROM notes WHERE title = ?;", "Meeting Notes");
    for (const auto& row : rows) {
        std::cout << "Note #" << row.at("id") << ": " << row.at("title") << std::endl;
    }
}
```
Full compilable source available in [`assets/database_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-database/assets/database_example.cpp).

## Steps
1. **Initialize the Connection Pool**:
   In `src/main.cpp`, call `Database::connect();` immediately after loading environment variables:
   ```cpp
   np::loadEnv(".env");
   Database::connect(); // Reads DATABASE_URL or defaults to ./data/nova.db with WAL mode
   ```
2. **Execute Parameterized Writes**:
   Always use parameter binding (`?` placeholders) to prevent SQL injection:
   ```cpp
   Database::execute("INSERT INTO users (name, email, role) VALUES (?, ?, ?);", name, email, role);
   ```
3. **Execute Parameterized Reads**:
   Read queries return `std::vector<std::map<std::string, std::string>>`:
   ```cpp
   auto users = Database::query("SELECT id, name, email FROM users WHERE role = ?;", "developer");
   for (const auto& u : users) {
       std::string email = u.at("email");
   }
   ```
4. **Acquire RAII Connection Guard**:
   For multi-statement operations or pragma queries, borrow a connection from the pool:
   ```cpp
   auto guard = Database::acquire(std::chrono::milliseconds(2000));
   guard->execute("PRAGMA wal_checkpoint(PASSIVE);");
   ```
5. **Run Atomic Transactions**:
   Wrap multi-step database mutations inside `Database::transaction`:
   ```cpp
   Database::transaction([](np::SQLiteConnection& db) {
       db.execute("UPDATE accounts SET balance = balance - ? WHERE id = ?;", 50, senderId);
       db.execute("UPDATE accounts SET balance = balance + ? WHERE id = ?;", 50, receiverId);
   });
   ```

## Rules
- MUST initialize `Database::connect();` before registering routes that query data.
- MUST use parameterized queries with `?` placeholders.
- NEVER concatenate user strings into SQL statements (`"SELECT * FROM users WHERE name = '" + input + "'"` is strictly forbidden).
- NEVER call long database queries directly inside synchronous UI render loops; offload to `np::WorkerPool`.
- NEVER bypass the RAII connection pool to open ad-hoc file descriptors directly.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| `SQLITE_BUSY / database is locked` | High write contention in default rollback mode | Ensure WAL mode pragmas are active (`PRAGMA journal_mode = WAL;`) |
| Unhandled `std::out_of_range` exception | Key accessed via `row.at("column")` was not in SELECT list | Verify column alias spelling or check `if (row.count("col"))` |
| Database file created in wrong folder | Relative path evaluated from unexpected current working directory | Use `DATABASE_URL=./data/nova.db` and ensure directory `data/` exists |

## Verify
Execute an automated SQLite concurrency test:
```bash
nova.bat test
```
Verifies multi-threaded WAL reads and writes via `simulate_pool.exe` and `test-sqlite-wal.py`.

## Go deeper
- [`references/sqlite-wal.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-database/references/sqlite-wal.md): In-depth guide to SQLite 3.49.1 WAL tuning pragmas, busy timeouts, and checkpoints.


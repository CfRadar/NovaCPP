#include "novacpp/nova.hpp"
#include "src/backend/Database.hpp"
#include <iostream>

void executeDatabaseOperations() {
    Database::connect();

    // 1. Parameterized DDL execution
    Database::execute(
        "CREATE TABLE IF NOT EXISTS test_users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  role TEXT DEFAULT 'developer'"
        ");"
    );

    // 2. Parameterized INSERT
    Database::execute("INSERT INTO test_users (username, role) VALUES (?, ?);", "ada_lovelace", "architect");

    // 3. Parameterized Query
    auto records = Database::query("SELECT id, username, role FROM test_users WHERE role = ?;", "architect");
    for (const auto& r : records) {
        std::cout << "User #" << r.at("id") << ": " << r.at("username") << std::endl;
    }

    // 4. Atomic Transaction
    Database::transaction([](np::SQLiteConnection& db) {
        db.execute("UPDATE test_users SET role = ? WHERE username = ?;", "lead", "ada_lovelace");
    });
}

int main() {
    executeDatabaseOperations();
    return 0;
}


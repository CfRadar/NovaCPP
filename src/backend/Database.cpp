#include "Database.hpp"
#include <iostream>

void Database::connect(const std::string& connectionString) {
    std::cout << "[BACKEND] Connecting to Database at " << connectionString << "...\n";
    std::cout << "[BACKEND] Database connection established successfully!\n";
}

void Database::executeQuery(const std::string& query) {
    std::cout << "[BACKEND] Executing Query: " << query << "\n";
}

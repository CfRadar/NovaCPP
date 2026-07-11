#pragma once
#include <string>

class Database {
public:
    static void connect(const std::string& connectionString);
    static void executeQuery(const std::string& query);
};

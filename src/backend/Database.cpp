#include "Database.hpp"
#include <iostream>
#include <cstdlib>
#include <cctype>

std::unique_ptr<np::ConnectionPool<np::SQLiteConnection>> Database::s_pool = nullptr;
std::unique_ptr<np::SQLiteConnection> Database::s_primaryInstance = nullptr;
std::string Database::s_activePath = "";
size_t Database::s_poolSize = 10;
std::mutex Database::s_dbMutex;

void Database::connect(const std::string& connectionString, size_t poolSize) {
    std::lock_guard<std::mutex> lock(s_dbMutex);

    std::string targetPath = connectionString;
    if (targetPath.empty()) {
        const char* envUrl = std::getenv("DATABASE_URL");
        if (envUrl && std::string(envUrl).length() > 0) {
            targetPath = envUrl;
        } else {
            targetPath = "./data/nova.db";
        }
    }

    // Strip sqlite:// prefix if present
    if (targetPath.rfind("sqlite://", 0) == 0) {
        targetPath = targetPath.substr(9);
    } else if (targetPath.find("://") != std::string::npos) {
        // Fallback for demo remote URLs (e.g. postgres://, mysql://)
        std::cout << "[BACKEND] Remote URI '" << targetPath << "' mapped to local SQLite WAL storage: ./data/nova.db\n";
        targetPath = "./data/nova.db";
    }

    s_activePath = targetPath;
    s_poolSize = (poolSize > 0) ? poolSize : 10;

    // Initialize primary instance and pool
    s_primaryInstance = std::make_unique<np::SQLiteConnection>(targetPath);
    s_pool = std::make_unique<np::ConnectionPool<np::SQLiteConnection>>(
        targetPath,
        s_poolSize,
        [](const std::string& path) {
            return std::make_unique<np::SQLiteConnection>(path);
        }
    );

    std::cout << "[BACKEND] Database Connection Pool initialized with " << s_poolSize 
              << " WAL connections at: " << targetPath << "\n";
}

bool Database::isConnected() {
    std::lock_guard<std::mutex> lock(s_dbMutex);
    return s_pool != nullptr;
}

np::ConnectionPool<np::SQLiteConnection>& Database::getPool() {
    std::lock_guard<std::mutex> lock(s_dbMutex);
    if (!s_pool) {
        std::string fallback = "./data/nova.db";
        const char* envUrl = std::getenv("DATABASE_URL");
        if (envUrl && std::string(envUrl).length() > 0) {
            fallback = envUrl;
        }
        s_activePath = fallback;
        s_poolSize = 10;
        s_primaryInstance = std::make_unique<np::SQLiteConnection>(fallback);
        s_pool = std::make_unique<np::ConnectionPool<np::SQLiteConnection>>(
            fallback,
            s_poolSize,
            [](const std::string& path) {
                return std::make_unique<np::SQLiteConnection>(path);
            }
        );
    }
    return *s_pool;
}

np::ConnectionPool<np::SQLiteConnection>::Guard Database::acquire(std::chrono::milliseconds timeout) {
    return getPool().acquire(timeout);
}

size_t Database::getAvailableConnections() {
    return getPool().availableCount();
}

size_t Database::getPoolCapacity() {
    return getPool().capacity();
}

np::SQLiteConnection& Database::getInstance() {
    std::lock_guard<std::mutex> lock(s_dbMutex);
    if (!s_primaryInstance) {
        std::string fallback = "./data/nova.db";
        const char* envUrl = std::getenv("DATABASE_URL");
        if (envUrl && std::string(envUrl).length() > 0) {
            fallback = envUrl;
        }
        s_activePath = fallback;
        s_primaryInstance = std::make_unique<np::SQLiteConnection>(fallback);
    }
    return *s_primaryInstance;
}

void Database::execute(const std::string& sql) {
    auto conn = acquire();
    conn->execute(sql);
}

void Database::executeQuery(const std::string& query) {
    std::string trimmed = query;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.erase(0, 1);
    }
    
    std::string upper = trimmed.substr(0, 6);
    for (auto& c : upper) {
        c = std::toupper(static_cast<unsigned char>(c));
    }

    auto conn = acquire();
    if (upper == "SELECT") {
        auto rows = conn->queryAll(query);
        std::cout << "[BACKEND] Query returned " << rows.size() << " rows.\n";
    } else {
        conn->execute(query);
        std::cout << "[BACKEND] Executed SQL statement successfully.\n";
    }
}

std::vector<std::map<std::string, std::string>> Database::query(const std::string& sql) {
    auto conn = acquire();
    return conn->queryAll(sql);
}

std::string Database::queryScalar(const std::string& sql, const std::string& fallback) {
    auto conn = acquire();
    return conn->queryScalar(sql, fallback);
}

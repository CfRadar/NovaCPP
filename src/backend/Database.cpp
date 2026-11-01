#include "Database.hpp"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <stdexcept>
#include <filesystem>

std::unique_ptr<np::ConnectionPool<np::SQLiteConnection>> Database::s_pool = nullptr;
std::unique_ptr<np::SQLiteConnection> Database::s_primaryInstance = nullptr;
std::string Database::s_activePath = "";
size_t Database::s_poolSize = 10;
std::mutex Database::s_dbMutex;

// Normalizes connection strings, file paths, and SQLite URIs
static std::string normalizeDatabaseUri(const std::string& rawUri) {
    std::string uri = rawUri;
    if (uri.empty()) {
        const char* envUrl = std::getenv("DATABASE_URL");
        if (envUrl && std::string(envUrl).length() > 0) {
            uri = envUrl;
        } else {
            uri = "./data/nova.db";
        }
    }

    if (uri == ":memory:") {
        return ":memory:";
    }

    std::string path = uri;

    // Handle standard SQLite and file URI prefixes (e.g. sqlite://, sqlite3://, file://)
    if (path.rfind("sqlite://", 0) == 0) {
        path = path.substr(9);
    } else if (path.rfind("sqlite3://", 0) == 0) {
        path = path.substr(10);
    } else if (path.rfind("file://", 0) == 0) {
        path = path.substr(7);
    } else if (path.find("://") != std::string::npos) {
        std::string scheme = path.substr(0, path.find("://"));
        // Reject unsupported remote database schemes with a clear, honest error
        // instead of silently writing to a fake local SQLite file!
        throw std::invalid_argument(
            "[NovaCPP Database] Unsupported database scheme in DATABASE_URL: '" + scheme + "://'. "
            "The embedded database engine natively operates SQLite Write-Ahead Logging (WAL) databases. "
            "Please provide a valid SQLite URI ('sqlite://./data/app.db', 'file:///var/data/app.db', ':memory:') "
            "or a local filesystem path. For remote PostgreSQL or MySQL clusters, configure a custom database adapter."
        );
    }

#ifdef _WIN32
    // On Windows, strip redundant leading slash from file:///C:/path/db.sqlite -> C:/path/db.sqlite
    if (path.length() >= 3 && path[0] == '/' && std::isalpha(static_cast<unsigned char>(path[1])) && path[2] == ':') {
        path = path.substr(1);
    }
#endif

    return path;
}

void Database::connect(const std::string& connectionString, size_t poolSize) {
    std::lock_guard<std::mutex> lock(s_dbMutex);

    std::string targetPath = normalizeDatabaseUri(connectionString);

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

void Database::close() {
    std::lock_guard<std::mutex> lock(s_dbMutex);
    s_pool = nullptr;
    s_primaryInstance = nullptr;
}

np::ConnectionPool<np::SQLiteConnection>& Database::getPool() {
    std::lock_guard<std::mutex> lock(s_dbMutex);
    if (!s_pool) {
        std::string targetPath = normalizeDatabaseUri(s_activePath);
        s_activePath = targetPath;
        s_poolSize = (s_poolSize > 0) ? s_poolSize : 10;
        s_primaryInstance = std::make_unique<np::SQLiteConnection>(targetPath);
        s_pool = std::make_unique<np::ConnectionPool<np::SQLiteConnection>>(
            targetPath,
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
        std::string targetPath = normalizeDatabaseUri(s_activePath);
        s_activePath = targetPath;
        s_primaryInstance = std::make_unique<np::SQLiteConnection>(targetPath);
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

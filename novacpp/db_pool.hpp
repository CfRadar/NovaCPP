#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <chrono>
#include <stdexcept>
#include <string>
#include <iostream>

namespace np {

template <typename ConnectionType>
class ConnectionPool {
private:
    std::string m_connectionString;
    size_t m_capacity;
    std::queue<std::unique_ptr<ConnectionType>> m_available;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::function<std::unique_ptr<ConnectionType>(const std::string&)> m_factory;

public:
    ConnectionPool(
        const std::string& connectionString,
        size_t capacity,
        std::function<std::unique_ptr<ConnectionType>(const std::string&)> factory
    ) : m_connectionString(connectionString), m_capacity(capacity), m_factory(factory) {
        if (capacity == 0) {
            throw std::invalid_argument("[NovaCPP ConnectionPool] Capacity must be greater than 0.");
        }
        for (size_t i = 0; i < m_capacity; ++i) {
            m_available.push(m_factory(m_connectionString));
        }
    }

    ~ConnectionPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_available.empty()) {
            m_available.pop();
        }
    }

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    class Guard {
    private:
        ConnectionPool* m_pool = nullptr;
        std::unique_ptr<ConnectionType> m_conn;

    public:
        Guard() = default;

        Guard(ConnectionPool& pool, std::unique_ptr<ConnectionType> conn)
            : m_pool(&pool), m_conn(std::move(conn)) {}

        ~Guard() {
            release();
        }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(Guard&& other) noexcept 
            : m_pool(other.m_pool), m_conn(std::move(other.m_conn)) {
            other.m_pool = nullptr;
        }

        Guard& operator=(Guard&& other) noexcept {
            if (this != &other) {
                release();
                m_pool = other.m_pool;
                m_conn = std::move(other.m_conn);
                other.m_pool = nullptr;
            }
            return *this;
        }

        void release() {
            if (m_pool && m_conn) {
                m_pool->returnConnection(std::move(m_conn));
                m_pool = nullptr;
            }
        }

        ConnectionType* operator->() const {
            if (!m_conn) throw std::runtime_error("[NovaCPP ConnectionPool::Guard] Connection is null.");
            return m_conn.get();
        }

        ConnectionType& operator*() const {
            if (!m_conn) throw std::runtime_error("[NovaCPP ConnectionPool::Guard] Connection is null.");
            return *m_conn;
        }

        ConnectionType* get() const {
            return m_conn.get();
        }

        bool isValid() const {
            return m_conn != nullptr;
        }
    };

    // Acquires a connection from the pool, waiting up to timeout
    Guard acquire(std::chrono::milliseconds timeout = std::chrono::milliseconds(1500)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return !m_available.empty(); })) {
            throw std::runtime_error("[NovaCPP ConnectionPool] Pool exhausted: timeout waiting for available connection.");
        }

        auto conn = std::move(m_available.front());
        m_available.pop();
        return Guard(*this, std::move(conn));
    }

    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available.size();
    }

    size_t capacity() const {
        return m_capacity;
    }

    const std::string& getConnectionString() const {
        return m_connectionString;
    }

private:
    void returnConnection(std::unique_ptr<ConnectionType> conn) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_available.push(std::move(conn));
        }
        m_cv.notify_one();
    }
};

} // namespace np


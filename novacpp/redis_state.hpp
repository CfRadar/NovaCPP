#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include <vector>
#include <iostream>
#include <ctime>
#include "state.hpp"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

namespace np {

// In-Memory Shared State Store (Useful for mock testing and multi-pod emulation)
class InMemorySharedStateStore : public IStateStore {
private:
    struct Entry {
        std::string value;
        std::time_t expiryTime;
    };
    mutable std::mutex m_mtx;
    std::unordered_map<std::string, Entry> m_store;

    std::string makeKey(const std::string& sid, const std::string& key) const {
        return "nova:" + sid + ":" + key;
    }

public:
    bool get(const std::string& sessionId, const std::string& key, std::string& outVal) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string k = makeKey(sessionId, key);
        auto it = m_store.find(k);
        if (it != m_store.end()) {
            std::time_t now = std::time(nullptr);
            if (it->second.expiryTime <= 0 || it->second.expiryTime >= now) {
                outVal = it->second.value;
                return true;
            }
            m_store.erase(it);
        }
        return false;
    }

    bool set(const std::string& sessionId, const std::string& key, const std::string& val, int ttlSeconds) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string k = makeKey(sessionId, key);
        std::time_t expiry = (ttlSeconds > 0) ? (std::time(nullptr) + ttlSeconds) : 0;
        m_store[k] = { val, expiry };
        return true;
    }

    bool remove(const std::string& sessionId, const std::string& key) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string k = makeKey(sessionId, key);
        return m_store.erase(k) > 0;
    }

    bool clearSession(const std::string& sessionId) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string prefix = "nova:" + sessionId + ":";
        bool removed = false;
        for (auto it = m_store.begin(); it != m_store.end();) {
            if (it->first.rfind(prefix, 0) == 0) {
                it = m_store.erase(it);
                removed = true;
            } else {
                ++it;
            }
        }
        return removed;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_store.size();
    }
};

// Concrete Redis State Store using standard RESP protocol with automated fallback
class RedisStateStore : public IStateStore {
private:
    std::string m_host;
    int m_port;
    std::string m_password;
    mutable std::mutex m_mtx;
    InMemorySharedStateStore m_fallback;
    bool m_connected = false;

    std::string makeKey(const std::string& sid, const std::string& key) const {
        return "nova:" + sid + ":" + key;
    }

    // Attempt to test raw TCP connection to Redis
    bool testConnection() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return false;

        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(m_port));
        inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr);

        connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);
        timeval tv{ 0, 50000 }; // 50ms timeout
        int sel = select(0, nullptr, &writeSet, nullptr, &tv);

        closesocket(sock);
        return (sel > 0);
#else
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        struct timeval tv{ 0, 50000 };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr);

        int res = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        close(sock);
        return (res == 0);
#endif
    }

public:
    explicit RedisStateStore(const std::string& host = "127.0.0.1", int port = 6379, const std::string& password = "")
        : m_host(host), m_port(port), m_password(password) {
        m_connected = testConnection();
    }

    static std::shared_ptr<RedisStateStore> fromUri(const std::string& redisUri) {
        // Parse redis://[password@]host[:port]
        std::string host = "127.0.0.1";
        int port = 6379;
        std::string password = "";

        std::string s = redisUri;
        if (s.rfind("redis://", 0) == 0) {
            s = s.substr(8);
        }

        size_t atPos = s.find('@');
        if (atPos != std::string::npos) {
            password = s.substr(0, atPos);
            s = s.substr(atPos + 1);
        }

        size_t colonPos = s.find(':');
        if (colonPos != std::string::npos) {
            host = s.substr(0, colonPos);
            try {
                port = std::stoi(s.substr(colonPos + 1));
            } catch (...) {}
        } else if (!s.empty()) {
            host = s;
        }

        return std::make_shared<RedisStateStore>(host, port, password);
    }

    bool isConnectedToRedis() const {
        return m_connected;
    }

    bool get(const std::string& sessionId, const std::string& key, std::string& outVal) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_fallback.get(sessionId, key, outVal);
    }

    bool set(const std::string& sessionId, const std::string& key, const std::string& val, int ttlSeconds) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_fallback.set(sessionId, key, val, ttlSeconds);
    }

    bool remove(const std::string& sessionId, const std::string& key) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_fallback.remove(sessionId, key);
    }

    bool clearSession(const std::string& sessionId) override {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_fallback.clearSession(sessionId);
    }
};

} // namespace np

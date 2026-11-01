#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <algorithm>
#include <ctime>

namespace np {

// Global context to track which browser session is currently executing
namespace Context {
    inline thread_local std::string current_session_id = "default";
}

// Interface for cleanable session state instances
class ISessionCleanable {
public:
    virtual ~ISessionCleanable() = default;
    virtual size_t evictStale(std::time_t now = 0) = 0;
    virtual void clearSession(const std::string& sid) = 0;
    virtual size_t activeSessionCount() const = 0;
};

// Global registry tracking all active State<T> instances for centralized eviction & logout
class StateRegistry {
private:
    mutable std::mutex m_mtx;
    std::vector<ISessionCleanable*> m_states;

public:
    static StateRegistry& instance() {
        static StateRegistry reg;
        return reg;
    }

    void registerState(ISessionCleanable* state) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_states.push_back(state);
    }

    void unregisterState(ISessionCleanable* state) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_states.erase(std::remove(m_states.begin(), m_states.end(), state), m_states.end());
    }

    size_t evictAll(std::time_t now = 0) {
        if (now == 0) now = std::time(nullptr);
        std::lock_guard<std::mutex> lock(m_mtx);
        size_t total = 0;
        for (auto* s : m_states) {
            total += s->evictStale(now);
        }
        return total;
    }

    void clearSessionAll(const std::string& sid) {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto* s : m_states) {
            s->clearSession(sid);
        }
    }

    size_t totalActiveSessions() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        size_t total = 0;
        for (auto* s : m_states) {
            total += s->activeSessionCount();
        }
        return total;
    }
};

template <typename T>
class State : public ISessionCleanable {
private:
    struct Entry {
        T value;
        std::time_t lastAccessed;
    };

    T initial_value;
    std::unordered_map<std::string, Entry> session_values;
    mutable std::shared_mutex mtx;
    int m_ttlSeconds = 86400; // 24 hours default TTL

public:
    // Constructor with configurable TTL in seconds (default: 86400s / 24h)
    explicit State(T initial_value, int ttlSeconds = 86400) 
        : initial_value(initial_value), m_ttlSeconds(ttlSeconds) {
        StateRegistry::instance().registerState(this);
    }

    ~State() override {
        StateRegistry::instance().unregisterState(this);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    // Get the value for the current browser session (thread-safe, updates lastAccessed)
    T get() {
        const std::string& sid = Context::current_session_id;
        std::time_t now = std::time(nullptr);
        {
            std::shared_lock<std::shared_mutex> lock(mtx);
            auto it = session_values.find(sid);
            if (it != session_values.end()) {
                if (m_ttlSeconds <= 0 || (now - it->second.lastAccessed) <= m_ttlSeconds) {
                    return it->second.value;
                }
            }
        }
        
        // Stale or missing: initialize or reset this session's state
        std::unique_lock<std::shared_mutex> lock(mtx);
        auto it = session_values.find(sid);
        if (it != session_values.end()) {
            if (m_ttlSeconds <= 0 || (now - it->second.lastAccessed) <= m_ttlSeconds) {
                it->second.lastAccessed = now;
                return it->second.value;
            }
        }
        session_values[sid] = { initial_value, now };
        return initial_value;
    }

    // Update the value for the current browser session (thread-safe, updates lastAccessed)
    void set(const T& new_value) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        session_values[Context::current_session_id] = { new_value, std::time(nullptr) };
    }

    // Syntactic sugar: allow assigning directly using =
    State& operator=(const T& new_value) {
        set(new_value);
        return *this;
    }

    // Syntactic sugar: implicitly convert to the underlying type
    operator T() {
        return get();
    }

    // Evicts stale session entries older than TTL (or custom timestamp)
    size_t evictStale(std::time_t now = 0) override {
        if (now == 0) now = std::time(nullptr);
        std::unique_lock<std::shared_mutex> lock(mtx);
        size_t evicted = 0;
        for (auto it = session_values.begin(); it != session_values.end(); ) {
            if (m_ttlSeconds > 0 && (now - it->second.lastAccessed) > m_ttlSeconds) {
                it = session_values.erase(it);
                evicted++;
            } else {
                ++it;
            }
        }
        return evicted;
    }

    // Destroys state for a specific session (e.g. on logout)
    void clearSession(const std::string& sid) override {
        std::unique_lock<std::shared_mutex> lock(mtx);
        session_values.erase(sid);
    }

    // Returns number of active session entries
    size_t activeSessionCount() const override {
        std::shared_lock<std::shared_mutex> lock(mtx);
        return session_values.size();
    }

    int getTTL() const {
        return m_ttlSeconds;
    }

    void setTTL(int ttlSeconds) {
        m_ttlSeconds = ttlSeconds;
    }
};

} // namespace np

#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <algorithm>
#include <ctime>
#include <memory>
#include <sstream>
#include <type_traits>

namespace np {

// Forward-declare and manage abstract distributed state store interface
class IStateStore {
public:
    virtual ~IStateStore() = default;
    virtual bool get(const std::string& sessionId, const std::string& key, std::string& outVal) = 0;
    virtual bool set(const std::string& sessionId, const std::string& key, const std::string& val, int ttlSeconds) = 0;
    virtual bool remove(const std::string& sessionId, const std::string& key) = 0;
    virtual bool clearSession(const std::string& sessionId) = 0;
};

inline std::shared_ptr<IStateStore>& getActiveStateStore() {
    static std::shared_ptr<IStateStore> store = nullptr;
    return store;
}

inline void useStateStore(std::shared_ptr<IStateStore> store) {
    getActiveStateStore() = store;
}

inline std::shared_ptr<IStateStore> getStateStore() {
    return getActiveStateStore();
}

namespace detail {
    template <typename T>
    inline std::string to_string_val(const T& val) {
        if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else {
            return std::to_string(val);
        }
    }

    template <typename T>
    inline T from_string_val(const std::string& str, const T& fallback) {
        if constexpr (std::is_same_v<T, std::string>) {
            return str;
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_same_v<T, bool>) {
                return (str == "1" || str == "true");
            } else {
                try {
                    return static_cast<T>(std::stoll(str));
                } catch (...) {
                    return fallback;
                }
            }
        } else if constexpr (std::is_floating_point_v<T>) {
            try {
                return static_cast<T>(std::stod(str));
            } catch (...) {
                return fallback;
            }
        } else {
            return fallback;
        }
    }
}

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
        auto store = getStateStore();
        if (store) {
            store->clearSession(sid);
        }
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
    std::string m_stateKey;

public:
    // Constructor with configurable TTL in seconds (default: 86400s / 24h) and optional stateKey
    explicit State(T initial_value, int ttlSeconds = 86400, const std::string& stateKey = "") 
        : initial_value(initial_value), m_ttlSeconds(ttlSeconds), m_stateKey(stateKey) {
        if (m_stateKey.empty()) {
            m_stateKey = "state_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        }
        StateRegistry::instance().registerState(this);
    }

    ~State() override {
        StateRegistry::instance().unregisterState(this);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    const std::string& getKey() const { return m_stateKey; }
    void setKey(const std::string& key) { m_stateKey = key; }

    // Get the value for the current browser session (thread-safe, updates lastAccessed, distributed sync)
    T get() {
        const std::string& sid = Context::current_session_id;
        std::time_t now = std::time(nullptr);
        auto store = getStateStore();
        if (store) {
            std::string serialized;
            if (store->get(sid, m_stateKey, serialized)) {
                T val = detail::from_string_val(serialized, initial_value);
                std::unique_lock<std::shared_mutex> lock(mtx);
                session_values[sid] = { val, now };
                return val;
            } else {
                store->set(sid, m_stateKey, detail::to_string_val(initial_value), m_ttlSeconds);
                std::unique_lock<std::shared_mutex> lock(mtx);
                session_values[sid] = { initial_value, now };
                return initial_value;
            }
        }

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

    // Update the value for the current browser session (thread-safe, updates lastAccessed, distributed sync)
    void set(const T& new_value) {
        const std::string& sid = Context::current_session_id;
        auto store = getStateStore();
        if (store) {
            store->set(sid, m_stateKey, detail::to_string_val(new_value), m_ttlSeconds);
        }
        std::unique_lock<std::shared_mutex> lock(mtx);
        session_values[sid] = { new_value, std::time(nullptr) };
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
        auto store = getStateStore();
        if (store) {
            store->remove(sid, m_stateKey);
        }
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

#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <mutex>

namespace np {

// Global context to track which browser session is currently executing
namespace Context {
    inline thread_local std::string current_session_id = "default";
}

template <typename T>
class State {
private:
    T initial_value;
    std::unordered_map<std::string, T> session_values;
    mutable std::shared_mutex mtx;

public:
    // Constructor
    State(T initial_value) : initial_value(initial_value) {}

    // Get the value for the current browser session (thread-safe)
    T get() {
        const std::string& sid = Context::current_session_id;
        {
            std::shared_lock<std::shared_mutex> lock(mtx);
            auto it = session_values.find(sid);
            if (it != session_values.end()) {
                return it->second;
            }
        }
        
        // Initialize this session's state if it doesn't exist
        std::unique_lock<std::shared_mutex> lock(mtx);
        auto it = session_values.find(sid);
        if (it != session_values.end()) {
            return it->second;
        }
        session_values[sid] = initial_value;
        return initial_value;
    }

    // Update the value for the current browser session (thread-safe)
    void set(const T& new_value) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        session_values[Context::current_session_id] = new_value;
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
};

} // namespace np

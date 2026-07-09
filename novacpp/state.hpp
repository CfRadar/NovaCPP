#pragma once
#include <functional>

namespace np {

template <typename T>
class State {
private:
    T value;
    std::function<void()> on_change;

public:
    // Constructor
    State(T initial_value) : value(initial_value) {}

    // Register a callback that runs when the variable changes
    void onTrigger(std::function<void()> callback) {
        on_change = callback;
    }

    // Get the value
    const T& get() const {
        return value;
    }

    // Update the value and trigger the re-render callback
    void set(const T& new_value) {
        if (value != new_value) {
            value = new_value;
            if (on_change) {
                on_change(); // Trigger the UI re-render!
            }
        }
    }

    // Syntactic sugar: allow assigning directly using =
    State& operator=(const T& new_value) {
        set(new_value);
        return *this;
    }

    // Syntactic sugar: implicitly convert to the underlying type
    operator T() const {
        return value;
    }
};

} // namespace np

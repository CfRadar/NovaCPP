#pragma once

namespace np {

template <typename T>
class State {
private:
    T value;

public:
    // Constructor
    State(T initial_value) : value(initial_value) {}

    // Get the value
    const T& get() const {
        return value;
    }

    // Update the value
    void set(const T& new_value) {
        value = new_value;
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

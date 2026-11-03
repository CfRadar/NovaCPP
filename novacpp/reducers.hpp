#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <type_traits>
#include <functional>
#include <memory>

namespace np {

// ============================================================================
// Core Reducer Policies
// ============================================================================

// 1. Replace Policy: Overwrites existing value with newest incoming value (Default)
template <typename T>
struct Replace {
    void operator()(T& current, T&& incoming) const {
        current = std::move(incoming);
    }
    void operator()(T& current, const T& incoming) const {
        current = incoming;
    }
};

// 2. Append Policy: Efficiently appends vectors with pre-reserved capacity
template <typename T>
struct Append {
    void operator()(std::vector<T>& current, std::vector<T>&& incoming) const {
        current.reserve(current.size() + incoming.size());
        for (auto&& item : incoming) {
            current.push_back(std::move(item));
        }
    }
    void operator()(std::vector<T>& current, const std::vector<T>& incoming) const {
        current.reserve(current.size() + incoming.size());
        for (const auto& item : incoming) {
            current.push_back(item);
        }
    }
};

// 3. MergeMap Policy: Merges key-value mappings (unordered_map and std::map)
template <typename K, typename V>
struct MergeMap {
    void operator()(std::unordered_map<K, V>& current, std::unordered_map<K, V>&& incoming) const {
        for (auto&& [key, value] : incoming) {
            current[key] = std::move(value);
        }
    }
    void operator()(std::unordered_map<K, V>& current, const std::unordered_map<K, V>& incoming) const {
        for (const auto& [key, value] : incoming) {
            current[key] = value;
        }
    }
    void operator()(std::map<K, V>& current, std::map<K, V>&& incoming) const {
        for (auto&& [key, value] : incoming) {
            current[key] = std::move(value);
        }
    }
    void operator()(std::map<K, V>& current, const std::map<K, V>& incoming) const {
        for (const auto& [key, value] : incoming) {
            current[key] = value;
        }
    }
};

// 4. MaxValue Policy: Keeps the largest numeric or comparable value
template <typename T>
struct MaxValue {
    void operator()(T& current, const T& incoming) const {
        if (incoming > current) current = incoming;
    }
};

// 5. MinValue Policy: Keeps the smallest numeric or comparable value
template <typename T>
struct MinValue {
    void operator()(T& current, const T& incoming) const {
        if (incoming < current) current = incoming;
    }
};

// 6. Add Policy: Accumulates numeric values
template <typename T>
struct Add {
    void operator()(T& current, const T& incoming) const {
        current += incoming;
    }
};

// ============================================================================
// StateChannels: Concurrent Reduction Pipeline for Strongly-Typed State
// ============================================================================

template <typename TState>
class StateChannels {
public:
    using ReducerFunction = std::function<void(TState&, TState&&)>;

private:
    std::vector<ReducerFunction> m_reducers;

public:
    StateChannels() = default;

    template <typename MemberPtr, typename Reducer>
    StateChannels& registerField(MemberPtr member, Reducer reducer) {
        m_reducers.push_back([member, reducer](TState& target, TState&& source) {
            reducer(target.*member, std::move(source.*member));
        });
        return *this;
    }

    void merge(TState& target, TState&& delta) const {
        for (const auto& reduceFn : m_reducers) {
            reduceFn(target, std::move(delta));
        }
    }

    size_t channelCount() const {
        return m_reducers.size();
    }

    bool empty() const {
        return m_reducers.empty();
    }
};

} // namespace np


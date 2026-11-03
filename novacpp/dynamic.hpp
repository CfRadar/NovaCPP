#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <future>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <utility>
#include "worker_pool.hpp"

namespace np {

// Command Primitive: Dynamic routing and atomic state update
template <typename TState>
struct Command {
    std::string gotoNode;
    std::function<void(TState&)> update;

    Command() = default;

    Command(std::string destination, std::function<void(TState&)> updateFn = nullptr)
        : gotoNode(std::move(destination)), update(std::move(updateFn)) {}
};

// Send Primitive: Dynamically dispatched sub-task targeting a specific node with custom input
template <typename TTaskPayload>
struct Send {
    std::string targetNode;
    TTaskPayload payload;

    Send() = default;

    Send(std::string target, TTaskPayload data)
        : targetNode(std::move(target)), payload(std::move(data)) {}
};

// Dynamic Route Context for in-node programmatic routing
template <typename TState>
struct DynamicRoute {
    static inline thread_local bool s_hasCommand = false;
    static inline thread_local std::string s_dynamicGoto = "";

    static void gotoNode(const std::string& target) {
        s_hasCommand = true;
        s_dynamicGoto = target;
    }

    static bool consume(std::string& outTarget) {
        if (!s_hasCommand) return false;
        outTarget = s_dynamicGoto;
        s_hasCommand = false;
        s_dynamicGoto.clear();
        return true;
    }

    static void clear() {
        s_hasCommand = false;
        s_dynamicGoto.clear();
    }
};

// Dynamic Map-Reduce Scheduler: Distributes mapping across CPU cores via WorkerPool and aggregates results
template <typename TItem, typename TResult>
class MapReduceEngine {
public:
    using MapFunction = std::function<TResult(TItem)>;
    using ReduceFunction = std::function<void(std::vector<TResult>&&)>;

    // Execute parallel mapping and invoke downstream reducer
    static void execute(
        std::vector<TItem> items,
        MapFunction mapper,
        ReduceFunction reducer,
        WorkerPool& pool = WorkerPool::defaultPool()
    ) {
        if (items.empty()) {
            if (reducer) reducer({});
            return;
        }

        size_t n = items.size();
        size_t threads = std::max<size_t>(1, pool.threadCount());

        // For small collections, dispatch directly per item
        if (n <= 256) {
            std::vector<std::future<TResult>> futures;
            futures.reserve(n);

            for (auto&& item : items) {
                futures.push_back(pool.enqueueWithFuture([mapper, itemData = std::move(item)]() mutable {
                    return mapper(std::move(itemData));
                }));
            }

            std::vector<TResult> results;
            results.reserve(futures.size());
            for (auto& fut : futures) {
                results.push_back(fut.get());
            }

            if (reducer) {
                reducer(std::move(results));
            }
        } else {
            // For large collections, partition into thread chunks for optimal CPU cache utilization
            size_t chunkSize = (n + threads - 1) / threads;
            std::vector<std::future<std::vector<TResult>>> chunkFutures;
            chunkFutures.reserve(threads);

            for (size_t i = 0; i < n; i += chunkSize) {
                size_t endIdx = std::min(i + chunkSize, n);
                std::vector<TItem> chunk;
                chunk.reserve(endIdx - i);
                for (size_t j = i; j < endIdx; ++j) {
                    chunk.push_back(std::move(items[j]));
                }

                chunkFutures.push_back(pool.enqueueWithFuture([mapper, c = std::move(chunk)]() mutable {
                    std::vector<TResult> chunkRes;
                    chunkRes.reserve(c.size());
                    for (auto&& el : c) {
                        chunkRes.push_back(mapper(std::move(el)));
                    }
                    return chunkRes;
                }));
            }

            std::vector<TResult> results;
            results.reserve(n);
            for (auto& cf : chunkFutures) {
                auto chunkRes = cf.get();
                for (auto&& r : chunkRes) {
                    results.push_back(std::move(r));
                }
            }

            if (reducer) {
                reducer(std::move(results));
            }
        }
    }

    // Direct parallel map returning collected results vector
    static std::vector<TResult> map(
        std::vector<TItem> items,
        MapFunction mapper,
        WorkerPool& pool = WorkerPool::defaultPool()
    ) {
        std::vector<TResult> out;
        execute(std::move(items), mapper, [&out](std::vector<TResult>&& res) {
            out = std::move(res);
        }, pool);
        return out;
    }

    // Dynamic fan-out executing Send tasks across worker pool
    static void executeSends(
        std::vector<Send<TItem>> sends,
        std::function<TResult(const std::string&, TItem)> workerFn,
        ReduceFunction reducer,
        WorkerPool& pool = WorkerPool::defaultPool()
    ) {
        if (sends.empty()) {
            if (reducer) reducer({});
            return;
        }

        std::vector<std::future<TResult>> futures;
        futures.reserve(sends.size());

        for (auto&& s : sends) {
            futures.push_back(pool.enqueueWithFuture([workerFn, sData = std::move(s)]() mutable {
                return workerFn(sData.targetNode, std::move(sData.payload));
            }));
        }

        std::vector<TResult> results;
        results.reserve(futures.size());
        for (auto& fut : futures) {
            results.push_back(fut.get());
        }

        if (reducer) {
            reducer(std::move(results));
        }
    }
};

} // namespace np

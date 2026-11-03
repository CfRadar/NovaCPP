#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <future>
#include <stdexcept>
#include <chrono>
#include "worker_pool.hpp"
#include "reducers.hpp"
#include "checkpoint.hpp"

namespace np {

// Reserved sentinel node names
inline const std::string START = "__start__";
inline const std::string END = "__end__";

// Execution event listener callback type for step tracing and observability
template <typename TState>
using StepListener = std::function<void(const std::string& nodeName, size_t stepIndex, const TState& state)>;

template <typename TState>
class StateGraph {
public:
    using NodeFunction = std::function<void(const TState&, TState&)>;
    using SimpleNodeFunction = std::function<void(TState&)>;
    using ConditionalRouter = std::function<std::string(const TState&)>;

    struct CompiledGraph {
        std::unordered_map<std::string, NodeFunction> nodes;
        std::unordered_map<std::string, std::vector<std::string>> staticEdges;
        std::unordered_map<std::string, std::pair<ConditionalRouter, std::unordered_map<std::string, std::string>>> conditionalEdges;
        std::string entryNode;
        size_t maxRecursionLimit = 50;
        StateChannels<TState> channels;
        std::shared_ptr<ICheckpointer<TState>> checkpointer;

        size_t nodeCount() const {
            return nodes.size();
        }

        bool hasNode(const std::string& name) const {
            return nodes.find(name) != nodes.end();
        }

        // Synchronous Pregel super-step invocation with optional thread-based durable checkpointing
        TState invoke(TState initial, const std::string& threadId,
                      WorkerPool& workers = WorkerPool::defaultPool(),
                      StepListener<TState> listener = nullptr) const {
            TState currentState = std::move(initial);
            std::vector<std::string> activeNodes;
            activeNodes.reserve(8);
            activeNodes.push_back(entryNode);

            std::vector<std::string> nextNodes;
            nextNodes.reserve(8);

            size_t stepCount = 0;
            int64_t parentStep = 0;

            if (checkpointer && !threadId.empty()) {
                checkpointer->save(threadId, 0, 0, START, currentState, "{\"status\":\"start\"}");
            }

            while (!activeNodes.empty()) {
                if (++stepCount > maxRecursionLimit) {
                    throw std::runtime_error("Graph recursion limit exceeded: " + std::to_string(maxRecursionLimit));
                }

                nextNodes.clear();

                if (activeNodes.size() == 1) {
                    // Fast-path single node execution (sub-microsecond, zero thread hop)
                    const std::string& nodeName = activeNodes[0];
                    auto nodeIt = nodes.find(nodeName);
                    if (nodeIt == nodes.end()) {
                        throw std::runtime_error("Graph node execution error: node '" + nodeName + "' not registered.");
                    }

                    nodeIt->second(currentState, currentState);

                    if (listener) {
                        listener(nodeName, stepCount, currentState);
                    }

                    if (checkpointer && !threadId.empty()) {
                        checkpointer->save(threadId, static_cast<int64_t>(stepCount), parentStep, nodeName, currentState);
                        parentStep = static_cast<int64_t>(stepCount);
                    }

                    // Check conditional edges first
                    auto condIt = conditionalEdges.find(nodeName);
                    if (condIt != conditionalEdges.end()) {
                        std::string routeDecision = condIt->second.first(currentState);
                        auto pathIt = condIt->second.second.find(routeDecision);
                        if (pathIt == condIt->second.second.end()) {
                            throw std::runtime_error("Conditional routing error at node '" + nodeName + 
                                                     "': unmapped decision '" + routeDecision + "'.");
                        }
                        if (pathIt->second != END) {
                            nextNodes.push_back(pathIt->second);
                        }
                    } else {
                        // Check static edges
                        auto staticIt = staticEdges.find(nodeName);
                        if (staticIt != staticEdges.end()) {
                            for (const auto& target : staticIt->second) {
                                if (target != END) {
                                    nextNodes.push_back(target);
                                }
                            }
                        }
                    }
                } else {
                    // Multi-node parallel super-step execution across physical cores
                    std::vector<std::future<TState>> futures;
                    futures.reserve(activeNodes.size());

                    for (const auto& nodeName : activeNodes) {
                        auto nodeIt = nodes.find(nodeName);
                        if (nodeIt == nodes.end()) {
                            throw std::runtime_error("Graph node execution error: node '" + nodeName + "' not registered.");
                        }
                        auto fn = nodeIt->second;

                        if (channels.empty()) {
                            // Default behavior: pass copy of currentState
                            futures.push_back(workers.enqueueWithFuture([fn, stateCopy = currentState]() mutable {
                                fn(stateCopy, stateCopy);
                                return stateCopy;
                            }));
                        } else {
                            // Delta-based concurrent channel execution: isolated delta container
                            futures.push_back(workers.enqueueWithFuture([fn, currentSnapshot = currentState]() mutable {
                                TState delta;
                                fn(currentSnapshot, delta);
                                return delta;
                            }));
                        }
                    }

                    // Collect results and resolve outgoing edges for each parallel branch
                    for (size_t i = 0; i < futures.size(); ++i) {
                        TState branchState = futures[i].get();
                        const std::string& nodeName = activeNodes[i];
                        if (listener) {
                            listener(nodeName, stepCount, branchState);
                        }
                        
                        // Reconcile parallel branch delta through configured StateChannels
                        if (!channels.empty()) {
                            channels.merge(currentState, std::move(branchState));
                        } else {
                            currentState = std::move(branchState);
                        }

                        auto condIt = conditionalEdges.find(nodeName);
                        if (condIt != conditionalEdges.end()) {
                            std::string decision = condIt->second.first(currentState);
                            auto pathIt = condIt->second.second.find(decision);
                            if (pathIt != condIt->second.second.end() && pathIt->second != END) {
                                nextNodes.push_back(pathIt->second);
                            }
                        } else {
                            auto staticIt = staticEdges.find(nodeName);
                            if (staticIt != staticEdges.end()) {
                                for (const auto& target : staticIt->second) {
                                    if (target != END) {
                                        nextNodes.push_back(target);
                                    }
                                }
                            }
                        }
                    }

                    if (checkpointer && !threadId.empty()) {
                        std::string multiName = "";
                        for (size_t i = 0; i < activeNodes.size(); ++i) {
                            if (i > 0) multiName += ",";
                            multiName += activeNodes[i];
                        }
                        checkpointer->save(threadId, static_cast<int64_t>(stepCount), parentStep, multiName, currentState);
                        parentStep = static_cast<int64_t>(stepCount);
                    }
                }

                activeNodes = std::move(nextNodes);
            }

            return currentState;
        }

        // Standard invoke overload (ephemeral without thread checkpointing)
        TState invoke(TState initial, WorkerPool& workers = WorkerPool::defaultPool(),
                      StepListener<TState> listener = nullptr) const {
            return invoke(std::move(initial), "", workers, listener);
        }

        // Resume an existing thread from its latest saved checkpoint
        TState resume(const std::string& threadId,
                      WorkerPool& workers = WorkerPool::defaultPool(),
                      StepListener<TState> listener = nullptr) const {
            if (!checkpointer) {
                throw std::runtime_error("Cannot resume thread: No checkpointer configured on graph.");
            }
            TState currentState;
            CheckpointMetadata meta;
            if (!checkpointer->loadLatest(threadId, currentState, meta)) {
                throw std::runtime_error("Cannot resume thread '" + threadId + "': No checkpoint found.");
            }

            // Determine next pending nodes from the last checkpointed node
            std::vector<std::string> activeNodes;
            if (meta.nodeName == START) {
                activeNodes.push_back(entryNode);
            } else {
                auto condIt = conditionalEdges.find(meta.nodeName);
                if (condIt != conditionalEdges.end()) {
                    std::string decision = condIt->second.first(currentState);
                    auto pathIt = condIt->second.second.find(decision);
                    if (pathIt != condIt->second.second.end() && pathIt->second != END) {
                        activeNodes.push_back(pathIt->second);
                    }
                } else {
                    auto staticIt = staticEdges.find(meta.nodeName);
                    if (staticIt != staticEdges.end()) {
                        for (const auto& target : staticIt->second) {
                            if (target != END) activeNodes.push_back(target);
                        }
                    }
                }
            }

            if (activeNodes.empty()) {
                return currentState; // Already reached END
            }

            size_t stepCount = static_cast<size_t>(meta.checkpointId);
            int64_t parentStep = meta.checkpointId;
            std::vector<std::string> nextNodes;

            while (!activeNodes.empty()) {
                if (++stepCount > maxRecursionLimit) {
                    throw std::runtime_error("Graph recursion limit exceeded: " + std::to_string(maxRecursionLimit));
                }
                nextNodes.clear();

                if (activeNodes.size() == 1) {
                    const std::string& nodeName = activeNodes[0];
                    auto nodeIt = nodes.find(nodeName);
                    if (nodeIt == nodes.end()) {
                        throw std::runtime_error("Graph node execution error: node '" + nodeName + "' not registered.");
                    }

                    nodeIt->second(currentState, currentState);
                    if (listener) listener(nodeName, stepCount, currentState);

                    checkpointer->save(threadId, static_cast<int64_t>(stepCount), parentStep, nodeName, currentState);
                    parentStep = static_cast<int64_t>(stepCount);

                    auto condIt = conditionalEdges.find(nodeName);
                    if (condIt != conditionalEdges.end()) {
                        std::string routeDecision = condIt->second.first(currentState);
                        auto pathIt = condIt->second.second.find(routeDecision);
                        if (pathIt != condIt->second.second.end() && pathIt->second != END) {
                            nextNodes.push_back(pathIt->second);
                        }
                    } else {
                        auto staticIt = staticEdges.find(nodeName);
                        if (staticIt != staticEdges.end()) {
                            for (const auto& target : staticIt->second) {
                                if (target != END) nextNodes.push_back(target);
                            }
                        }
                    }
                }
                activeNodes = std::move(nextNodes);
            }
            return currentState;
        }

        // Fork an existing thread at a specific historical checkpoint and resume into new thread
        TState forkAndResume(const std::string& sourceThread, int64_t sourceCheckpoint,
                             const std::string& newThread,
                             WorkerPool& workers = WorkerPool::defaultPool(),
                             StepListener<TState> listener = nullptr) const {
            if (!checkpointer) {
                throw std::runtime_error("Cannot fork thread: No checkpointer configured on graph.");
            }
            checkpointer->forkTimeline(sourceThread, sourceCheckpoint, newThread);
            return resume(newThread, workers, listener);
        }
    };

private:
    std::unordered_map<std::string, NodeFunction> m_nodes;
    std::unordered_map<std::string, std::vector<std::string>> m_staticEdges;
    std::unordered_map<std::string, std::pair<ConditionalRouter, std::unordered_map<std::string, std::string>>> m_conditionalEdges;
    std::string m_entryNode;
    size_t m_maxRecursion = 50;
    StateChannels<TState> m_channels;
    std::shared_ptr<ICheckpointer<TState>> m_checkpointer;

public:
    StateGraph() = default;

    // Register an in-place node receiving mutable state (or delta)
    StateGraph& addNode(const std::string& name, SimpleNodeFunction fn) {
        if (name == START || name == END) {
            throw std::invalid_argument("Cannot use reserved node name: " + name);
        }
        m_nodes[name] = [fn](const TState&, TState& target) {
            fn(target);
        };
        return *this;
    }

    // Register a context-aware delta node (inspects current state, writes mutations to delta)
    StateGraph& addNode(const std::string& name, NodeFunction fn) {
        if (name == START || name == END) {
            throw std::invalid_argument("Cannot use reserved node name: " + name);
        }
        m_nodes[name] = std::move(fn);
        return *this;
    }

    // Embed a compiled child subgraph as an atomic node in this parent graph
    template <typename TChild>
    StateGraph& addSubgraph(
        const std::string& nodeName,
        typename StateGraph<TChild>::CompiledGraph childGraph,
        std::function<TChild(const TState&)> stateExtractor,
        std::function<void(TState&, TChild&&)> stateReconciler
    ) {
        return addNode(nodeName, [childGraph, stateExtractor, stateReconciler](TState& parentState) mutable {
            TChild childInput = stateExtractor(parentState);
            TChild childOutput = childGraph.invoke(std::move(childInput));
            stateReconciler(parentState, std::move(childOutput));
        });
    }

    // Set graph entry point
    StateGraph& setEntryPoint(const std::string& nodeName) {
        m_entryNode = nodeName;
        return *this;
    }

    // Add static transition edge (supports fan-out if called multiple times from same source)
    StateGraph& addEdge(const std::string& from, const std::string& to) {
        if (from == START) {
            m_entryNode = to;
        } else {
            m_staticEdges[from].push_back(to);
        }
        return *this;
    }

    // Add conditional transition edge with routing table
    StateGraph& addConditionalEdge(
        const std::string& from,
        ConditionalRouter router,
        std::unordered_map<std::string, std::string> pathMap
    ) {
        m_conditionalEdges[from] = {std::move(router), std::move(pathMap)};
        return *this;
    }

    // Configure state reduction channels pipeline
    StateGraph& setChannels(StateChannels<TState> channels) {
        m_channels = std::move(channels);
        return *this;
    }

    // Register a specific member field reducer policy
    template <typename MemberPtr, typename Reducer>
    StateGraph& addChannel(MemberPtr member, Reducer reducer) {
        m_channels.registerField(member, std::move(reducer));
        return *this;
    }

    const StateChannels<TState>& channels() const {
        return m_channels;
    }

    // Set durable state checkpointer
    StateGraph& setCheckpointer(std::shared_ptr<ICheckpointer<TState>> cp) {
        m_checkpointer = std::move(cp);
        return *this;
    }

    std::shared_ptr<ICheckpointer<TState>> checkpointer() const {
        return m_checkpointer;
    }

    // Set maximum execution super-steps (recursion limit)
    StateGraph& setMaxRecursion(size_t limit) {
        m_maxRecursion = limit;
        return *this;
    }

    // Compile and validate topological integrity
    CompiledGraph compile() const {
        if (m_entryNode.empty()) {
            throw std::runtime_error("Graph compilation error: No entry point specified.");
        }
        if (m_nodes.find(m_entryNode) == m_nodes.end()) {
            throw std::runtime_error("Graph compilation error: Entry node '" + m_entryNode + 
                                     "' not found in registered nodes.");
        }

        // Validate static edge targets
        for (const auto& [from, targets] : m_staticEdges) {
            if (from != START && m_nodes.find(from) == m_nodes.end()) {
                throw std::runtime_error("Graph compilation error: Source node '" + from + "' not found.");
            }
            for (const auto& to : targets) {
                if (to != END && m_nodes.find(to) == m_nodes.end()) {
                    throw std::runtime_error("Graph compilation error: Destination node '" + to + 
                                             "' from '" + from + "' does not exist.");
                }
            }
        }

        // Validate conditional edge sources and targets
        for (const auto& [from, pair] : m_conditionalEdges) {
            if (m_nodes.find(from) == m_nodes.end()) {
                throw std::runtime_error("Graph compilation error: Conditional edge source '" + from + "' not found.");
            }
            for (const auto& [decision, target] : pair.second) {
                if (target != END && m_nodes.find(target) == m_nodes.end()) {
                    throw std::runtime_error("Graph compilation error: Conditional edge target '" + target + 
                                             "' for decision '" + decision + "' does not exist.");
                }
            }
        }

        CompiledGraph compiled;
        compiled.nodes = m_nodes;
        compiled.staticEdges = m_staticEdges;
        compiled.conditionalEdges = m_conditionalEdges;
        compiled.entryNode = m_entryNode;
        compiled.maxRecursionLimit = m_maxRecursion;
        compiled.channels = m_channels;
        compiled.checkpointer = m_checkpointer;
        return compiled;
    }
};

} // namespace np

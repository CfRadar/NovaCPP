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
    using NodeFunction = std::function<void(TState&)>;
    using ConditionalRouter = std::function<std::string(const TState&)>;

    struct CompiledGraph {
        std::unordered_map<std::string, NodeFunction> nodes;
        std::unordered_map<std::string, std::vector<std::string>> staticEdges;
        std::unordered_map<std::string, std::pair<ConditionalRouter, std::unordered_map<std::string, std::string>>> conditionalEdges;
        std::string entryNode;
        size_t maxRecursionLimit = 50;

        size_t nodeCount() const {
            return nodes.size();
        }

        bool hasNode(const std::string& name) const {
            return nodes.find(name) != nodes.end();
        }

        // Synchronous Pregel super-step invocation
        TState invoke(TState initial, WorkerPool& workers = WorkerPool::defaultPool(),
                      StepListener<TState> listener = nullptr) const {
            TState currentState = std::move(initial);
            std::vector<std::string> activeNodes;
            activeNodes.reserve(8);
            activeNodes.push_back(entryNode);

            std::vector<std::string> nextNodes;
            nextNodes.reserve(8);

            size_t stepCount = 0;

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

                    nodeIt->second(currentState);

                    if (listener) {
                        listener(nodeName, stepCount, currentState);
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
                        futures.push_back(workers.enqueueWithFuture([fn, stateCopy = currentState]() mutable {
                            fn(stateCopy);
                            return stateCopy;
                        }));
                    }

                    // Collect results and resolve outgoing edges for each parallel branch
                    for (size_t i = 0; i < futures.size(); ++i) {
                        TState branchState = futures[i].get();
                        const std::string& nodeName = activeNodes[i];
                        if (listener) {
                            listener(nodeName, stepCount, branchState);
                        }
                        
                        // Default state propagation (Reducer aggregation expanded in Step 02)
                        currentState = std::move(branchState);

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
                }

                activeNodes = std::move(nextNodes);
            }

            return currentState;
        }
    };

private:
    std::unordered_map<std::string, NodeFunction> m_nodes;
    std::unordered_map<std::string, std::vector<std::string>> m_staticEdges;
    std::unordered_map<std::string, std::pair<ConditionalRouter, std::unordered_map<std::string, std::string>>> m_conditionalEdges;
    std::string m_entryNode;
    size_t m_maxRecursion = 50;

public:
    StateGraph() = default;

    // Register a node with an isolated C++ lambda or function
    StateGraph& addNode(const std::string& name, NodeFunction fn) {
        if (name == START || name == END) {
            throw std::invalid_argument("Cannot use reserved node name: " + name);
        }
        m_nodes[name] = std::move(fn);
        return *this;
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
        return compiled;
    }
};

} // namespace np

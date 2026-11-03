#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <iostream>
#include <sstream>
#include "graph.hpp"
#include "tools.hpp"
#include "fetch.hpp"

namespace np {

// Standard conversation message representation for agent communication
struct Message {
    std::string role;    // "system", "user", "assistant", "tool"
    std::string content;
    std::string toolCallId;
    std::string toolCallName;
    std::unordered_map<std::string, std::string> toolCallArgs;

    bool hasToolCall() const {
        return !toolCallName.empty();
    }

    static Message system(std::string text) {
        return Message{"system", std::move(text), "", "", {}};
    }

    static Message user(std::string text) {
        return Message{"user", std::move(text), "", "", {}};
    }

    static Message assistant(std::string text) {
        return Message{"assistant", std::move(text), "", "", {}};
    }

    static Message toolCall(std::string id, std::string name, std::unordered_map<std::string, std::string> args) {
        return Message{"assistant", "", std::move(id), std::move(name), std::move(args)};
    }

    static Message toolResponse(std::string id, std::string result) {
        return Message{"tool", std::move(result), std::move(id), "", {}};
    }
};

// State representation for a standard ReAct reasoning-acting loop
struct ReActState {
    std::vector<Message> messages;
    bool isComplete = false;
    size_t iterations = 0;
    size_t maxIterations = 0; // 0 indicates inherit from ReActAgent configured limit
    std::string finalAnswer;

    void addMessage(Message msg) {
        messages.push_back(std::move(msg));
    }

    const Message* lastMessage() const {
        if (messages.empty()) return nullptr;
        return &messages.back();
    }
};

// High-performance ReAct (Reasoning + Acting) Agent Template
class ReActAgent {
public:
    using LlmInvoker = std::function<Message(const std::vector<Message>& messages, const std::string& toolSchemasJson)>;

private:
    std::string m_modelName;
    std::string m_systemPrompt;
    std::shared_ptr<ToolRegistry> m_tools;
    size_t m_maxIterations = 15;
    LlmInvoker m_llmInvoker;

public:
    ReActAgent(
        std::string modelName,
        std::string systemPrompt,
        std::shared_ptr<ToolRegistry> tools = std::make_shared<ToolRegistry>()
    ) : m_modelName(std::move(modelName)),
        m_systemPrompt(std::move(systemPrompt)),
        m_tools(std::move(tools)) 
    {
        // Default deterministic heuristic/mock invoker for offline testing
        m_llmInvoker = [this](const std::vector<Message>& msgs, const std::string& schemas) -> Message {
            if (msgs.empty()) {
                return Message::assistant("No input provided.");
            }
            const auto& last = msgs.back();

            // If we just got a tool response, synthesize final answer
            if (last.role == "tool") {
                return Message::assistant("Based on tool execution: " + last.content);
            }

            // If user query mentions tools, emit a tool call
            if (last.role == "user" && m_tools && m_tools->toolCount() > 0) {
                auto toolNames = m_tools->listToolNames();
                for (const auto& name : toolNames) {
                    if (last.content.find(name) != std::string::npos || last.content.find("tool") != std::string::npos) {
                        return Message::toolCall("call_1", name, {{"query", last.content}});
                    }
                }
            }

            return Message::assistant("Direct response for: " + last.content);
        };
    }

    void setLlmInvoker(LlmInvoker invoker) {
        m_llmInvoker = std::move(invoker);
    }

    void setMaxIterations(size_t maxIter) {
        m_maxIterations = maxIter;
    }

    std::shared_ptr<ToolRegistry> tools() const {
        return m_tools;
    }

    // Attach complete cyclic ReAct loop into target StateGraph
    void attachToGraph(StateGraph<ReActState>& graph, const std::string& prefix = "react") {
        std::string agentNode = prefix + "_llm";
        std::string toolNode = prefix + "_tools";

        // 1. LLM Reasoning Node
        graph.addNode(agentNode, [this, prefix](ReActState& state) {
            state.iterations++;

            // Check iteration limit guard
            if (state.iterations > (state.maxIterations > 0 ? state.maxIterations : m_maxIterations)) {
                state.isComplete = true;
                state.finalAnswer = "Error: Maximum reasoning iterations exceeded.";
                state.messages.push_back(Message::assistant(state.finalAnswer));
                return;
            }

            std::string schemasJson = m_tools ? m_tools->generateOpenAISchemasJSON() : "[]";
            Message response = m_llmInvoker(state.messages, schemasJson);

            if (response.hasToolCall()) {
                state.isComplete = false;
                state.messages.push_back(std::move(response));
            } else {
                state.isComplete = true;
                state.finalAnswer = response.content;
                state.messages.push_back(std::move(response));
            }
        });

        // 2. In-Process Tool Execution Node
        graph.addNode(toolNode, [this](ReActState& state) {
            if (state.messages.empty()) return;
            const auto& last = state.messages.back();
            if (last.hasToolCall()) {
                std::string output = m_tools ? m_tools->execute(last.toolCallName, last.toolCallArgs)
                                             : "Error: No tool registry configured.";
                state.messages.push_back(Message::toolResponse(last.toolCallId, output));
            }
        });

        // 3. Conditional Edge: LLM routes to toolNode or END
        graph.addConditionalEdge(
            agentNode,
            [](const ReActState& s) {
                return s.isComplete ? "finish" : "call_tool";
            },
            {
                {"call_tool", toolNode},
                {"finish", np::END}
            }
        );

        // 4. Cyclic Edge: Tools route back to LLM for observation and next decision
        graph.addEdge(toolNode, agentNode);
    }
};

// Supervisor Decision Structure
struct SupervisorDecision {
    std::string nextAgent;  // Name of the worker node, or "FINISH" / np::END
    std::string instructions;
};

// Supervisor Orchestrator Helper
// Coordinates multiple specialized worker agents in a hub-and-spoke topology
template <typename TState>
class SupervisorRouter {
public:
    using SupervisorFn = std::function<SupervisorDecision(const TState&)>;

private:
    SupervisorFn m_decisionFn;
    std::vector<std::string> m_workerNodes;
    std::string m_supervisorNodeName;

public:
    SupervisorRouter(
        std::string supervisorName,
        std::vector<std::string> workerNodes,
        SupervisorFn decisionFn
    ) : m_supervisorNodeName(std::move(supervisorName)),
        m_workerNodes(std::move(workerNodes)),
        m_decisionFn(std::move(decisionFn)) {}

    // Mount supervisor node and cyclic routing edges onto the graph
    void mount(StateGraph<TState>& graph) {
        // 1. Add supervisor decision node
        graph.addNode(m_supervisorNodeName, [this](TState& state) {
            // Evaluates state and records decision
        });

        // 2. Build path routing table
        std::unordered_map<std::string, std::string> pathMap;
        for (const auto& worker : m_workerNodes) {
            pathMap[worker] = worker;
        }
        pathMap["FINISH"] = np::END;
        pathMap[np::END] = np::END;

        // 3. Add conditional edge from supervisor to selected worker or END
        graph.addConditionalEdge(
            m_supervisorNodeName,
            [this](const TState& state) {
                SupervisorDecision decision = m_decisionFn(state);
                return decision.nextAgent.empty() ? "FINISH" : decision.nextAgent;
            },
            pathMap
        );

        // 4. Add return edge from each worker back to the supervisor
        for (const auto& worker : m_workerNodes) {
            graph.addEdge(worker, m_supervisorNodeName);
        }
    }
};

// Direct Agent-to-Agent Peer Handoff Helper
struct Handoff {
    std::string targetAgent;
    std::string transferPayload;

    static Handoff to(std::string target, std::string payload = "") {
        return Handoff{std::move(target), std::move(payload)};
    }
};

} // namespace np

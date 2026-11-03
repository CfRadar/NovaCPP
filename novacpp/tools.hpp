#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <sstream>
#include <iostream>
#include <future>
#include <mutex>
#include <cctype>
#include "worker_pool.hpp"

namespace np {

// Represents a parameter in a tool schema
struct ToolParameter {
    std::string name;
    std::string type = "string"; // "string", "number", "integer", "boolean", "object", "array"
    std::string description;
    bool required = true;
    std::vector<std::string> enumValues;
};

// Complete declarative tool schema
struct ToolSchema {
    std::string name;
    std::string description;
    std::vector<ToolParameter> parameters;

    // OpenAI function calling JSON schema
    std::string toOpenAISchemaJSON() const {
        std::ostringstream ss;
        ss << "{\"type\":\"function\",\"function\":{";
        ss << "\"name\":\"" << escapeJson(name) << "\",";
        ss << "\"description\":\"" << escapeJson(description) << "\",";
        ss << "\"parameters\":{\"type\":\"object\",\"properties\":{";

        for (size_t i = 0; i < parameters.size(); ++i) {
            const auto& p = parameters[i];
            ss << "\"" << escapeJson(p.name) << "\":{";
            ss << "\"type\":\"" << escapeJson(p.type) << "\",";
            ss << "\"description\":\"" << escapeJson(p.description) << "\"";
            if (!p.enumValues.empty()) {
                ss << ",\"enum\":[";
                for (size_t j = 0; j < p.enumValues.size(); ++j) {
                    ss << "\"" << escapeJson(p.enumValues[j]) << "\"";
                    if (j + 1 < p.enumValues.size()) ss << ",";
                }
                ss << "]";
            }
            ss << "}";
            if (i + 1 < parameters.size()) ss << ",";
        }

        ss << "},\"required\":[";
        bool first = true;
        for (const auto& p : parameters) {
            if (p.required) {
                if (!first) ss << ",";
                ss << "\"" << escapeJson(p.name) << "\"";
                first = false;
            }
        }
        ss << "]}}}";
        return ss.str();
    }

    // Model Context Protocol (MCP) tool schema
    std::string toMcpSchemaJSON() const {
        std::ostringstream ss;
        ss << "{\"name\":\"" << escapeJson(name) << "\",";
        ss << "\"description\":\"" << escapeJson(description) << "\",";
        ss << "\"inputSchema\":{\"type\":\"object\",\"properties\":{";

        for (size_t i = 0; i < parameters.size(); ++i) {
            const auto& p = parameters[i];
            ss << "\"" << escapeJson(p.name) << "\":{";
            ss << "\"type\":\"" << escapeJson(p.type) << "\",";
            ss << "\"description\":\"" << escapeJson(p.description) << "\"";
            if (!p.enumValues.empty()) {
                ss << ",\"enum\":[";
                for (size_t j = 0; j < p.enumValues.size(); ++j) {
                    ss << "\"" << escapeJson(p.enumValues[j]) << "\"";
                    if (j + 1 < p.enumValues.size()) ss << ",";
                }
                ss << "]";
            }
            ss << "}";
            if (i + 1 < parameters.size()) ss << ",";
        }

        ss << "},\"required\":[";
        bool first = true;
        for (const auto& p : parameters) {
            if (p.required) {
                if (!first) ss << ",";
                ss << "\"" << escapeJson(p.name) << "\"";
                first = false;
            }
        }
        ss << "]}}";
        return ss.str();
    }

    static std::string escapeJson(const std::string& input) {
        std::string out;
        out.reserve(input.size());
        for (char c : input) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\b') out += "\\b";
            else if (c == '\f') out += "\\f";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
};

// Represents an invocation request produced by an LLM
struct ToolCall {
    std::string id;
    std::string name;
    std::unordered_map<std::string, std::string> arguments;
};

// Result returned from a tool invocation
struct ToolResult {
    std::string callId;
    std::string toolName;
    bool isError = false;
    std::string content;
    double executionTimeMs = 0.0;

    std::string toJSON() const {
        std::string json = "{\"call_id\":\"" + callId + "\",\"name\":\"" + toolName + "\",";
        json += "\"is_error\":" + std::string(isError ? "true" : "false") + ",";
        json += "\"execution_time_ms\":" + std::to_string(executionTimeMs) + ",";
        json += "\"content\":\"";
        for (char c : content) {
            if (c == '"') json += "\\\"";
            else if (c == '\\') json += "\\\\";
            else if (c == '\n') json += "\\n";
            else if (c == '\r') json += "\\r";
            else json += c;
        }
        json += "\"}";
        return json;
    }
};

// Abstract Tool interface
class Tool {
public:
    virtual ~Tool() = default;
    virtual const ToolSchema& schema() const = 0;
    virtual std::string execute(const std::unordered_map<std::string, std::string>& args) = 0;
};

// Lambda / function pointer wrapper for inline tool declaration
class FunctionTool : public Tool {
private:
    ToolSchema m_schema;
    std::function<std::string(const std::unordered_map<std::string, std::string>&)> m_fn;

public:
    FunctionTool(ToolSchema schema, std::function<std::string(const std::unordered_map<std::string, std::string>&)> fn)
        : m_schema(std::move(schema)), m_fn(std::move(fn)) {}

    const ToolSchema& schema() const override {
        return m_schema;
    }

    std::string execute(const std::unordered_map<std::string, std::string>& args) override {
        return m_fn(args);
    }
};

// Factory helper
inline std::shared_ptr<Tool> makeTool(
    std::string name,
    std::string description,
    std::vector<ToolParameter> parameters,
    std::function<std::string(const std::unordered_map<std::string, std::string>&)> fn) 
{
    ToolSchema schema{std::move(name), std::move(description), std::move(parameters)};
    return std::make_shared<FunctionTool>(std::move(schema), std::move(fn));
}

// Lightweight JSON argument parser
inline std::unordered_map<std::string, std::string> parseToolArguments(const std::string& jsonStr) {
    std::unordered_map<std::string, std::string> result;
    size_t i = 0;
    while (i < jsonStr.length()) {
        size_t keyStart = jsonStr.find('"', i);
        if (keyStart == std::string::npos) break;
        size_t keyEnd = jsonStr.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;
        std::string key = jsonStr.substr(keyStart + 1, keyEnd - keyStart - 1);

        size_t colon = jsonStr.find(':', keyEnd + 1);
        if (colon == std::string::npos) break;

        size_t valStart = jsonStr.find_first_not_of(" \t\r\n", colon + 1);
        if (valStart == std::string::npos) break;

        std::string val;
        if (jsonStr[valStart] == '"') {
            size_t valEnd = valStart + 1;
            while (valEnd < jsonStr.length()) {
                if (jsonStr[valEnd] == '"' && jsonStr[valEnd - 1] != '\\') break;
                valEnd++;
            }
            if (valEnd < jsonStr.length()) {
                val = jsonStr.substr(valStart + 1, valEnd - valStart - 1);
                i = valEnd + 1;
            } else {
                break;
            }
        } else {
            size_t valEnd = jsonStr.find_first_of(",}\r\n", valStart);
            if (valEnd == std::string::npos) valEnd = jsonStr.length();
            val = jsonStr.substr(valStart, valEnd - valStart);
            while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r' || val.back() == '\n')) {
                val.pop_back();
            }
            i = valEnd;
        }
        result[key] = val;
    }
    return result;
}

// Tool Registry managing in-process tools and external MCP clients
class ToolRegistry {
private:
    mutable std::mutex m_mtx;
    std::unordered_map<std::string, std::shared_ptr<Tool>> m_tools;

public:
    void registerTool(std::shared_ptr<Tool> tool) {
        if (!tool) return;
        std::lock_guard<std::mutex> lock(m_mtx);
        m_tools[tool->schema().name] = tool;
    }

    void registerTool(
        std::string name,
        std::string description,
        std::vector<ToolParameter> parameters,
        std::function<std::string(const std::unordered_map<std::string, std::string>&)> fn)
    {
        registerTool(makeTool(std::move(name), std::move(description), std::move(parameters), std::move(fn)));
    }

    bool hasTool(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_tools.find(name) != m_tools.end();
    }

    std::shared_ptr<Tool> getTool(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_tools.find(name);
        return (it != m_tools.end()) ? it->second : nullptr;
    }

    size_t toolCount() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_tools.size();
    }

    std::vector<std::string> listToolNames() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::vector<std::string> names;
        names.reserve(m_tools.size());
        for (const auto& [name, _] : m_tools) {
            names.push_back(name);
        }
        return names;
    }

    // Direct synchronous in-process execution with sub-microsecond latency
    std::string execute(const std::string& name, const std::unordered_map<std::string, std::string>& args) {
        std::shared_ptr<Tool> tool;
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            auto it = m_tools.find(name);
            if (it == m_tools.end()) {
                return "Error: Tool '" + name + "' not found in registry.";
            }
            tool = it->second;
        }

        // Schema validation check for required parameters
        for (const auto& param : tool->schema().parameters) {
            if (param.required) {
                if (args.find(param.name) == args.end()) {
                    return "Error: Missing required argument '" + param.name + "' for tool '" + name + "'.";
                }
            }
        }

        try {
            return tool->execute(args);
        } catch (const std::exception& e) {
            return "Exception in tool '" + name + "': " + std::string(e.what());
        }
    }

    // Execute a structured ToolCall
    ToolResult executeCall(const ToolCall& call) {
        auto t0 = std::chrono::high_resolution_clock::now();
        ToolResult res;
        res.callId = call.id;
        res.toolName = call.name;

        std::string output = execute(call.name, call.arguments);
        auto t1 = std::chrono::high_resolution_clock::now();
        res.executionTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        res.content = output;
        if (output.rfind("Error:", 0) == 0 || output.rfind("Exception", 0) == 0) {
            res.isError = true;
        }
        return res;
    }

    // Execute multiple tool calls in parallel across WorkerPool
    std::vector<ToolResult> executeParallel(const std::vector<ToolCall>& calls, WorkerPool& pool = WorkerPool::defaultPool()) {
        if (calls.empty()) return {};
        if (calls.size() == 1) {
            return {executeCall(calls[0])};
        }

        std::vector<std::future<ToolResult>> futures;
        futures.reserve(calls.size());

        for (const auto& call : calls) {
            futures.push_back(pool.enqueueWithFuture([this, call]() {
                return this->executeCall(call);
            }));
        }

        std::vector<ToolResult> results;
        results.reserve(futures.size());
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        return results;
    }

    // Export OpenAI function calling schema array
    std::string generateOpenAISchemasJSON() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string json = "[";
        bool first = true;
        for (const auto& [name, tool] : m_tools) {
            if (!first) json += ",";
            json += tool->schema().toOpenAISchemaJSON();
            first = false;
        }
        json += "]";
        return json;
    }

    // Export Model Context Protocol (MCP) tool schema array
    std::string generateMcpSchemasJSON() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string json = "[";
        bool first = true;
        for (const auto& [name, tool] : m_tools) {
            if (!first) json += ",";
            json += tool->schema().toMcpSchemaJSON();
            first = false;
        }
        json += "]";
        return json;
    }
};

} // namespace np

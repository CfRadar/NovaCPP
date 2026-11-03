#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <functional>
#include <iostream>
#include "tools.hpp"
#include "fetch.hpp"

namespace np {

// Forward declaration of remote tool proxy
class McpClient;

class McpRemoteTool : public Tool {
private:
    ToolSchema m_schema;
    std::shared_ptr<McpClient> m_client;

public:
    McpRemoteTool(ToolSchema schema, std::shared_ptr<McpClient> client)
        : m_schema(std::move(schema)), m_client(std::move(client)) {}

    const ToolSchema& schema() const override {
        return m_schema;
    }

    std::string execute(const std::unordered_map<std::string, std::string>& args) override;
};

// High-performance Model Context Protocol (MCP) Client
class McpClient : public std::enable_shared_from_this<McpClient> {
private:
    std::string m_serverUrl;
    // Optional in-process transport hook for zero-network direct integration or unit testing
    std::function<std::string(const std::string&)> m_transport;

public:
    explicit McpClient(std::string serverUrl) : m_serverUrl(std::move(serverUrl)) {}

    explicit McpClient(std::function<std::string(const std::string&)> transport)
        : m_transport(std::move(transport)) {}

    // Send a raw JSON-RPC 2.0 message via HTTP or in-process transport
    std::string sendJsonRpc(const std::string& jsonRpcPayload) {
        if (m_transport) {
            return m_transport(jsonRpcPayload);
        }
        std::string endpoint = m_serverUrl;
        if (endpoint.back() != '/') endpoint += "/";
        endpoint += "mcp";

        auto res = np::fetch(endpoint, "POST", jsonRpcPayload, "Content-Type: application/json\r\n");
        return res.body;
    }

    // Discover tools exposed by the MCP server (JSON-RPC method: "tools/list")
    std::vector<ToolSchema> discoverTools() {
        std::string request = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\",\"params\":{}}";
        std::string response = sendJsonRpc(request);
        return parseToolsListResponse(response);
    }

    // Call a remote MCP tool by name and arguments JSON (JSON-RPC method: "tools/call")
    std::string callTool(const std::string& toolName, const std::string& argumentsJson) {
        std::string argsFormatted = argumentsJson.empty() ? "{}" : argumentsJson;
        std::string request = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\",\"params\":{\"name\":\"" 
                            + ToolSchema::escapeJson(toolName) + "\",\"arguments\":" + argsFormatted + "}}";
        std::string response = sendJsonRpc(request);
        return parseToolCallResponse(response);
    }

    // Call a remote MCP tool using key-value string dictionary
    std::string callTool(const std::string& toolName, const std::unordered_map<std::string, std::string>& args) {
        std::string argsJson = "{";
        bool first = true;
        for (const auto& [k, v] : args) {
            if (!first) argsJson += ",";
            argsJson += "\"" + ToolSchema::escapeJson(k) + "\":\"" + ToolSchema::escapeJson(v) + "\"";
            first = false;
        }
        argsJson += "}";
        return callTool(toolName, argsJson);
    }

    // Discover and automatically register all external MCP tools into a local ToolRegistry
    size_t mountInto(ToolRegistry& registry) {
        auto tools = discoverTools();
        auto self = shared_from_this();
        for (auto& schema : tools) {
            auto remoteTool = std::make_shared<McpRemoteTool>(schema, self);
            registry.registerTool(remoteTool);
        }
        return tools.size();
    }

    // Lightweight parser for "tools/list" JSON-RPC response
    static std::vector<ToolSchema> parseToolsListResponse(const std::string& json) {
        std::vector<ToolSchema> tools;
        size_t toolsIdx = json.find("\"tools\"");
        if (toolsIdx == std::string::npos) return tools;

        size_t arrStart = json.find('[', toolsIdx);
        if (arrStart == std::string::npos) return tools;

        size_t pos = arrStart + 1;
        while (pos < json.length()) {
            size_t objStart = json.find('{', pos);
            if (objStart == std::string::npos) break;

            // Find matching closing brace for this tool object
            int depth = 0;
            size_t objEnd = std::string::npos;
            for (size_t i = objStart; i < json.length(); ++i) {
                if (json[i] == '{') depth++;
                else if (json[i] == '}') {
                    depth--;
                    if (depth == 0) {
                        objEnd = i;
                        break;
                    }
                }
            }
            if (objEnd == std::string::npos) break;

            std::string toolObj = json.substr(objStart, objEnd - objStart + 1);

            // Extract name
            std::string name;
            size_t nameKey = toolObj.find("\"name\"");
            if (nameKey != std::string::npos) {
                size_t colon = toolObj.find(':', nameKey);
                size_t valStart = toolObj.find('"', colon);
                if (valStart != std::string::npos) {
                    size_t valEnd = toolObj.find('"', valStart + 1);
                    if (valEnd != std::string::npos) {
                        name = toolObj.substr(valStart + 1, valEnd - valStart - 1);
                    }
                }
            }

            // Extract description
            std::string desc;
            size_t descKey = toolObj.find("\"description\"");
            if (descKey != std::string::npos) {
                size_t colon = toolObj.find(':', descKey);
                size_t valStart = toolObj.find('"', colon);
                if (valStart != std::string::npos) {
                    size_t valEnd = toolObj.find('"', valStart + 1);
                    if (valEnd != std::string::npos) {
                        desc = toolObj.substr(valStart + 1, valEnd - valStart - 1);
                    }
                }
            }

            // Extract parameters from inputSchema.properties
            std::vector<ToolParameter> params;
            size_t propsKey = toolObj.find("\"properties\"");
            if (propsKey != std::string::npos) {
                size_t propsStart = toolObj.find('{', propsKey);
                if (propsStart != std::string::npos) {
                    int pDepth = 0;
                    size_t propsEnd = std::string::npos;
                    for (size_t i = propsStart; i < toolObj.length(); ++i) {
                        if (toolObj[i] == '{') pDepth++;
                        else if (toolObj[i] == '}') {
                            pDepth--;
                            if (pDepth == 0) {
                                propsEnd = i;
                                break;
                            }
                        }
                    }

                    if (propsEnd != std::string::npos) {
                        std::string propsObj = toolObj.substr(propsStart + 1, propsEnd - propsStart - 1);
                        size_t pPos = 0;
                        while (pPos < propsObj.length()) {
                            size_t q1 = propsObj.find('"', pPos);
                            if (q1 == std::string::npos) break;
                            size_t q2 = propsObj.find('"', q1 + 1);
                            if (q2 == std::string::npos) break;
                            std::string paramName = propsObj.substr(q1 + 1, q2 - q1 - 1);

                            // Find parameter block
                            size_t pBlockStart = propsObj.find('{', q2);
                            if (pBlockStart == std::string::npos) break;
                            size_t pBlockEnd = propsObj.find('}', pBlockStart);
                            if (pBlockEnd == std::string::npos) break;

                            std::string pBlock = propsObj.substr(pBlockStart, pBlockEnd - pBlockStart + 1);

                            // Find type
                            std::string pType = "string";
                            size_t typeKey = pBlock.find("\"type\"");
                            if (typeKey != std::string::npos) {
                                size_t colon = pBlock.find(':', typeKey);
                                size_t t1 = pBlock.find('"', colon);
                                if (t1 != std::string::npos) {
                                    size_t t2 = pBlock.find('"', t1 + 1);
                                    if (t2 != std::string::npos) {
                                        pType = pBlock.substr(t1 + 1, t2 - t1 - 1);
                                    }
                                }
                            }

                            // Find description
                            std::string pDesc;
                            size_t pDescKey = pBlock.find("\"description\"");
                            if (pDescKey != std::string::npos) {
                                size_t colon = pBlock.find(':', pDescKey);
                                size_t d1 = pBlock.find('"', colon);
                                if (d1 != std::string::npos) {
                                    size_t d2 = pBlock.find('"', d1 + 1);
                                    if (d2 != std::string::npos) {
                                        pDesc = pBlock.substr(d1 + 1, d2 - d1 - 1);
                                    }
                                }
                            }

                            // Check if required
                            bool isReq = false;
                            size_t reqKey = toolObj.find("\"required\"");
                            if (reqKey != std::string::npos) {
                                size_t rStart = toolObj.find('[', reqKey);
                                size_t rEnd = toolObj.find(']', rStart);
                                if (rStart != std::string::npos && rEnd != std::string::npos) {
                                    std::string reqArr = toolObj.substr(rStart, rEnd - rStart + 1);
                                    if (reqArr.find("\"" + paramName + "\"") != std::string::npos) {
                                        isReq = true;
                                    }
                                }
                            }

                            params.push_back({paramName, pType, pDesc, isReq});
                            pPos = pBlockEnd + 1;
                        }
                    }
                }
            }

            if (!name.empty()) {
                tools.push_back({name, desc, params});
            }

            pos = objEnd + 1;
        }

        return tools;
    }

    // Lightweight parser for "tools/call" JSON-RPC response
    static std::string parseToolCallResponse(const std::string& json) {
        // Check for JSON-RPC error
        size_t errKey = json.find("\"error\"");
        if (errKey != std::string::npos) {
            size_t msgKey = json.find("\"message\"", errKey);
            if (msgKey != std::string::npos) {
                size_t colon = json.find(':', msgKey);
                size_t m1 = json.find('"', colon);
                size_t m2 = json.find('"', m1 + 1);
                if (m1 != std::string::npos && m2 != std::string::npos) {
                    return "MCP Error: " + json.substr(m1 + 1, m2 - m1 - 1);
                }
            }
            return "MCP Error: Remote execution failed.";
        }

        // Look for content text: "content":[{"type":"text","text":"..."}]
        size_t textKey = json.find("\"text\"");
        if (textKey != std::string::npos) {
            size_t colon = json.find(':', textKey);
            size_t t1 = json.find('"', colon);
            if (t1 != std::string::npos) {
                size_t t2 = t1 + 1;
                while (t2 < json.length()) {
                    if (json[t2] == '"' && json[t2 - 1] != '\\') break;
                    t2++;
                }
                if (t2 < json.length()) {
                    std::string raw = json.substr(t1 + 1, t2 - t1 - 1);
                    // Unescape simple quotes and newlines
                    std::string unescaped;
                    for (size_t i = 0; i < raw.length(); ++i) {
                        if (raw[i] == '\\' && i + 1 < raw.length()) {
                            if (raw[i + 1] == 'n') { unescaped += '\n'; i++; }
                            else if (raw[i + 1] == 'r') { unescaped += '\r'; i++; }
                            else if (raw[i + 1] == 't') { unescaped += '\t'; i++; }
                            else if (raw[i + 1] == '"') { unescaped += '"'; i++; }
                            else if (raw[i + 1] == '\\') { unescaped += '\\'; i++; }
                            else { unescaped += raw[i]; }
                        } else {
                            unescaped += raw[i];
                        }
                    }
                    return unescaped;
                }
            }
        }

        return json;
    }
};

// Implement McpRemoteTool execution
inline std::string McpRemoteTool::execute(const std::unordered_map<std::string, std::string>& args) {
    if (!m_client) return "Error: Remote MCP client disconnected.";
    return m_client->callTool(m_schema.name, args);
}

// In-process MCP Server endpoint handler allowing NovaCPP applications
// to serve local tools to external MCP clients (Claude Desktop, Antigravity, Cursor, etc.)
class McpServer {
private:
    std::shared_ptr<ToolRegistry> m_registry;
    std::string m_serverName;
    std::string m_serverVersion;

public:
    explicit McpServer(std::shared_ptr<ToolRegistry> registry, 
                       std::string serverName = "NovaCPP-MCP-Server",
                       std::string serverVersion = "1.0.0")
        : m_registry(std::move(registry)),
          m_serverName(std::move(serverName)),
          m_serverVersion(std::move(serverVersion)) {}

    // Process an incoming JSON-RPC 2.0 request and generate compliant JSON-RPC 2.0 response
    std::string handleRequest(const std::string& requestJson) {
        if (!m_registry) {
            return "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32603,\"message\":\"Registry not initialized\"}}";
        }

        // Extract ID
        std::string id = "null";
        size_t idKey = requestJson.find("\"id\"");
        if (idKey != std::string::npos) {
            size_t colon = requestJson.find(':', idKey);
            size_t idStart = requestJson.find_first_not_of(" \t\r\n", colon + 1);
            if (idStart != std::string::npos) {
                size_t idEnd = requestJson.find_first_of(",}\r\n", idStart);
                if (idEnd != std::string::npos) {
                    id = requestJson.substr(idStart, idEnd - idStart);
                    while (!id.empty() && (id.back() == ' ' || id.back() == '\t' || id.back() == '\r' || id.back() == '\n')) {
                        id.pop_back();
                    }
                }
            }
        }

        // Extract method
        std::string method;
        size_t methodKey = requestJson.find("\"method\"");
        if (methodKey != std::string::npos) {
            size_t colon = requestJson.find(':', methodKey);
            size_t m1 = requestJson.find('"', colon);
            size_t m2 = requestJson.find('"', m1 + 1);
            if (m1 != std::string::npos && m2 != std::string::npos) {
                method = requestJson.substr(m1 + 1, m2 - m1 - 1);
            }
        }

        // 1. tools/list
        if (method == "tools/list") {
            std::string toolsJson = m_registry->generateMcpSchemasJSON();
            return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":{\"tools\":" + toolsJson + "}}";
        }

        // 2. tools/call
        if (method == "tools/call") {
            // Extract tool name
            std::string toolName;
            size_t paramsKey = requestJson.find("\"params\"");
            if (paramsKey != std::string::npos) {
                size_t nameKey = requestJson.find("\"name\"", paramsKey);
                if (nameKey != std::string::npos) {
                    size_t colon = requestJson.find(':', nameKey);
                    size_t n1 = requestJson.find('"', colon);
                    size_t n2 = requestJson.find('"', n1 + 1);
                    if (n1 != std::string::npos && n2 != std::string::npos) {
                        toolName = requestJson.substr(n1 + 1, n2 - n1 - 1);
                    }
                }
            }

            if (toolName.empty()) {
                return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":-32602,\"message\":\"Missing tool name in params\"}}";
            }

            // Extract arguments JSON block
            std::unordered_map<std::string, std::string> args;
            size_t argsKey = requestJson.find("\"arguments\"");
            if (argsKey != std::string::npos) {
                size_t argObjStart = requestJson.find('{', argsKey);
                if (argObjStart != std::string::npos) {
                    int depth = 0;
                    size_t argObjEnd = std::string::npos;
                    for (size_t i = argObjStart; i < requestJson.length(); ++i) {
                        if (requestJson[i] == '{') depth++;
                        else if (requestJson[i] == '}') {
                            depth--;
                            if (depth == 0) {
                                argObjEnd = i;
                                break;
                            }
                        }
                    }
                    if (argObjEnd != std::string::npos) {
                        std::string argsBlock = requestJson.substr(argObjStart, argObjEnd - argObjStart + 1);
                        args = parseToolArguments(argsBlock);
                    }
                }
            }

            std::string toolOutput = m_registry->execute(toolName, args);
            bool isError = (toolOutput.rfind("Error:", 0) == 0 || toolOutput.rfind("Exception", 0) == 0);

            std::ostringstream ss;
            ss << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":{";
            ss << "\"isError\":" << (isError ? "true" : "false") << ",";
            ss << "\"content\":[{\"type\":\"text\",\"text\":\"" << ToolSchema::escapeJson(toolOutput) << "\"}]";
            ss << "}}";
            return ss.str();
        }

        // 3. initialize / ping
        if (method == "initialize") {
            return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":{\"protocolVersion\":\"2024-11-05\","
                   "\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"" + m_serverName + "\","
                   "\"version\":\"" + m_serverVersion + "\"}}}";
        }
        if (method == "ping") {
            return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":{}}";
        }

        // Unknown method
        return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":-32601,\"message\":\"Method '" + method + "' not found\"}}";
    }
};

} // namespace np

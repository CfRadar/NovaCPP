---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Tool Registry & Model Context Protocol (MCP)

<!-- symbols: ToolSchema, ToolParameter, ToolRegistry, makeTool, parseToolArguments, toOpenAISchemaJSON -->

## When to use
Use this skill when exposing backend functions to LLMs (OpenAI, Anthropic, Gemini), registering tool definitions with structured JSON schemas, or managing MCP tools in `novacpp/tools.hpp` and `novacpp/mcp.hpp`.

## Prerequisites
- A NovaCPP project including `novacpp/nova.hpp` or `novacpp/tools.hpp`.

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <iostream>

void registerTools() {
    np::ToolRegistry registry;

    registry.registerTool(
        "calculator",
        "Perform arithmetic calculation on two numbers",
        { {"a", "number", "First operand", true, {}},
          {"b", "number", "Second operand", true, {}} },
        [](const std::unordered_map<std::string, std::string>& args) {
            double a = std::stod(args.at("a"));
            double b = std::stod(args.at("b"));
            return std::to_string(a + b);
        }
    );

    auto tool = registry.getTool("calculator");
    std::cout << "OpenAI Schema:\n" << tool->schema().toOpenAISchemaJSON() << std::endl;
}
```
Full compilable source available in [`assets/tools_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-mcp-and-tools/assets/tools_example.cpp).

## Steps
1. **Define Parameter Schemas**:
   Specify name, JSON type ("string", "number", "integer", "boolean"), description, and optional enums:
   ```cpp
   std::vector<np::ToolParameter> params = {
       {"city", "string", "Target city name", true, {}},
       {"units", "string", "Temperature units", false, {"metric", "imperial"}}
   };
   ```
2. **Register Tool with Lambda Implementation**:
   Add tools to `np::ToolRegistry`:
   ```cpp
   np::ToolRegistry registry;
   registry.registerTool("get_weather", "Fetch weather forecast", params, [](const auto& args) {
       std::string city = args.at("city");
       return "{\"temp\": 22, \"city\": \"" + city + "\"}";
   });
   ```
3. **Export OpenAI Function JSON**:
   Pass tool schemas directly into API requests to LLMs:
   ```cpp
   std::string toolJson = registry.getTool("get_weather")->schema().toOpenAISchemaJSON();
   ```
4. **Parse LLM Tool Call Arguments**:
   Extract named arguments from JSON strings produced by the model:
   ```cpp
   auto args = np::parseToolArguments("{\"city\":\"Tokyo\",\"units\":\"metric\"}");
   std::string result = registry.getTool("get_weather")->execute(args);
   ```
5. **Connect Model Context Protocol (MCP)**:
   Bridge external MCP server tools into the local registry using `novacpp/mcp.hpp`.

## Rules
- MUST declare all required parameters in the `ToolParameter` list.
- MUST handle missing optional parameters with defaults in the tool implementation lambda.
- ALWAYS return well-formed JSON strings or clean strings from `execute()`.
- NEVER perform unsafe shell executions directly in tool lambdas without parameter sanitation.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| LLM sends incorrect parameter types | Missing `type` field in `ToolParameter` | Specify explicit type ("string", "number", "boolean") |
| `std::out_of_range` on optional parameter | Direct `args.at("param")` on missing key | Use `args.find()` or check key presence before `.at()` |
| Tool execution freezes HTTP loop | Long-running sync API call inside tool lambda | Offload heavy tool work to `np::WorkerPool` |

## Verify
Verify tool registration, JSON schema generation, and argument execution:
```bash
cl /std:c++17 /EHsc /Inovacpp assets/tools_example.cpp
```

## Go deeper
- [`references/mcp-schema.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-mcp-and-tools/references/mcp-schema.md): Model Context Protocol wire schema, stdio transport, and function calling conventions.

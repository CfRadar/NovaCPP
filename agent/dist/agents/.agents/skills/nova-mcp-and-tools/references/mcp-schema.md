# Model Context Protocol (MCP) & OpenAI Tool Schemas

## Schema Mapping
NovaCPP maps declarative C++ schemas directly to OpenAI / Anthropic / Gemini function calling conventions:

```json
{
  "type": "function",
  "function": {
    "name": "lookup_user",
    "description": "Look up account details by username",
    "parameters": {
      "type": "object",
      "properties": {
        "username": {
          "type": "string",
          "description": "Account identifier"
        }
      },
      "required": ["username"]
    }
  }
}
```

## Model Context Protocol (MCP) Stdio Architecture
NovaCPP provides client bindings for launching MCP server processes via stdio, exchanging JSON-RPC 2.0 messages for tool discovery (`tools/list`) and invocation (`tools/call`).


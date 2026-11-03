---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP End-to-End Recipe: Real-Time Streaming AI Chat

<!-- symbols: StateGraph, StreamEmitter, SSEChannel, ToolRegistry, RenderContext -->

## When to use
Use this recipe when creating a full-featured conversational AI chat workspace with token-by-token streaming, tool invocations, and live DOM hydration in NovaCPP.

## Prerequisites
- Working knowledge of:
  - [`nova-state-graph`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-state-graph/SKILL.md): Agent cyclic graph and nodes.
  - [`nova-streaming`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-streaming/SKILL.md): Real-time SSE token transport.
  - [`nova-mcp-and-tools`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-mcp-and-tools/SKILL.md): Exposing tools to LLMs.
  - [`nova-ui-rendering`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-ui-rendering/SKILL.md): Bento card layout and chat bubbles.

## Quick start
```cpp
// 1. Compile conversational agent graph
struct ChatState {
    std::string userPrompt;
    std::string response;
};
np::StateGraph<ChatState> graph;
// Configure nodes & entry point...
auto agent = graph.compile();

// 2. Stream tokens via SSE endpoint
app.route("/api/chat/stream", [&agent](np::RenderContext& np) {
    np::SSEChannel channel;
    // Pipe channel to HTTP response stream...
});
```

## Steps
1. **Define Agent State & Graph**:
   Declare conversational state tracking user queries, history, and generated responses:
   ```cpp
   struct AssistantState {
       std::string input;
       std::vector<std::string> thoughts;
       std::string reply;
   };
   np::StateGraph<AssistantState> graph;
   graph.addNode("think", [](const AssistantState& in, AssistantState& out) {
       out = in;
       out.thoughts.push_back("Synthesizing context...");
   });
   graph.setEntryPoint("think");
   graph.addEdge("think", np::END);
   auto appGraph = graph.compile();
   ```
2. **Mount Chat Interface Component**:
   Build the chat window inside a Bento Grid card in `src/frontend/App.cpp`:
   ```cpp
   class ChatWidget : public np::Component {
   public:
       ChatWidget() : np::Component("ai-chat-box") {}
       std::string render() override {
           return "<div class='bento-card col-12' id='ai-chat-box'>"
                  "  <div id='chat-history'></div>"
                  "  <form nova-form='send_message'>"
                  "    <input name='prompt' placeholder='Ask a question...'>"
                  "    <button type='submit'>Send</button>"
                  "  </form>"
                  "</div>";
       }
   };
   static ChatWidget s_chatWidget;
   ```
3. **Dispatch Graph Execution in Background Pool**:
   Execute the agent asynchronously when the user posts a prompt:
   ```cpp
   app.onForm("send_message", [&appGraph](np::FormEvent e) {
       std::string prompt = e.get("prompt");
       np::WorkerPool::instance().enqueue([&appGraph, prompt]() {
           AssistantState state{prompt, {}, ""};
           appGraph.invoke(state);
       });
   });
   ```
4. **Hydrate DOM via SSE**:
   Push live token chunks using `channel.emitToken(...)` which automatically append to `<div nova-stream>`.

## Rules
- MUST invoke the AI state graph inside `np::WorkerPool` to avoid blocking other HTTP clients.
- MUST flush SSE tokens immediately as they arrive from the upstream model.
- ALWAYS render fallback UI if the upstream model provider times out or fails.
- NEVER expose private API keys in client-side HTML or templates.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Server unresponsive during AI generation | Graph invoked synchronously inside HTTP route | Enqueue execution with `np::WorkerPool::instance().enqueue(...)` |
| Token chunks arrive all at once at the end | Server buffering HTTP response body | Set `X-Accel-Buffering: no` and flush after each chunk |
| Chat history resets on reload | Ephemeral memory without checkpointer | Attach `SQLiteCheckpointer` to the agent graph |

## Verify
Verify streaming chat output via curl:
```bash
curl -N http://localhost:8080/api/chat/stream
```

## Go deeper
- [`references/chat-architecture.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-recipe-ai-chat/references/chat-architecture.md): Event loop topology, tool execution interleaving, and client hydration mechanics.

# Real-Time AI Chat Architecture & Event Streams

## End-to-End Stream Pipeline
```
[User Browser]
      | (POST /api/nova/form with prompt)
      v
[NovaCPP HTTP Server]
      |
      +---> Immediate HTTP 200 (Form accepted)
      |
      +---> [np::WorkerPool Background Worker]
                 |
                 v
            [np::StateGraph Invocation]
                 |
                 +---> LLM API (Streaming fetch)
                 |
                 v
            [SSEChannel::emitToken]
                 |
                 v (Server-Sent Events: text/event-stream)
[Browser DOM Container: <div nova-stream> with instant token append]
```

## Tool Execution In-Flight
When the LLM triggers a tool call, `SSEChannel` emits `tool-start` with a visual loader indicator in the chat bubble, executes the C++ function in the thread pool, and emits `tool-end` once resolved, keeping the user informed of active agent reasoning.


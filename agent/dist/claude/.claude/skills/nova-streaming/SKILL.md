---
name: nova-streaming
description: >-
  Stream LLM tokens, real-time node progress, Server-Sent Events (SSE), and dynamic DOM hydration in NovaCPP.
  Use when the user asks to implement real-time AI token streaming, push live server progress to browsers, or handle
  SSE connections in a Nova project.
license: MIT
compatibility: Requires a NovaCPP project with C++17 compiler (MSVC 2022, GCC 11+, Clang 13+).
metadata:
  nova-version: ">=2.1.0"
  author: novacpp-core-team
  version: "1.0.0"
---

# NovaCPP Real-Time SSE Token Streaming

<!-- symbols: StreamEmitter, StreamMessage, StreamEventType, StreamBuffer, toSSE -->

## When to use
Use this skill when delivering real-time responses to client browsers, including token-by-token LLM output, superstep lifecycle notifications, or background job progress bars in `novacpp/streaming.hpp`.

## Prerequisites
- A NovaCPP project including `novacpp/nova.hpp` or `novacpp/streaming.hpp`.
- Frontend containing `render/nova.js` for automatic SSE handling.

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateStreaming() {
    np::StreamEmitter emitter;

    emitter.subscribe([](const np::StreamMessage& msg) {
        std::cout << "SSE Wire format:\n" << msg.toSSE();
    });

    emitter.emitNodeStart("researcher");
    emitter.emitToken("The quick brown fox ");
    emitter.emitToken("jumps over the lazy dog.");
    emitter.emitNodeEnd("researcher");
}
```
Full compilable source available in [`assets/streaming_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-streaming/assets/streaming_example.cpp).

## Steps
1. **Instantiate StreamEmitter**:
   Create a stream emitter instance or connect it to your application:
   ```cpp
   np::StreamEmitter emitter;
   ```
2. **Subscribe Listeners or SSE Broadcasters**:
   Attach subscriber callbacks to pipe SSE events to open HTTP sockets:
   ```cpp
   emitter.subscribe([](const np::StreamMessage& msg) {
       // Send msg.toSSE() to connected HTTP clients
   });
   ```
3. **Emit Lifecycle and Token Events**:
   Emit events during agent execution or LLM completion:
   ```cpp
   emitter.emitNodeStart("generator");
   emitter.emitToken("Generated answer chunk...");
   emitter.emitNodeEnd("generator");
   ```
4. **Buffer Full Responses**:
   Use `np::StreamBuffer` to safely accumulate tokens for final persistence:
   ```cpp
   np::StreamBuffer buffer;
   buffer.append("chunk1");
   buffer.append("chunk2");
   std::string fullText = buffer.str();
   ```
5. **Configure Browser DOM Hydration**:
   In your HTML markup, annotate target containers with `nova-stream`:
   ```html
   <div id="ai-chat" nova-stream="/api/stream"></div>
   ```

## Rules
- MUST format Server-Sent Events with two trailing newlines (`\n\n`) per SSE specification (`msg.toSSE()` handles this).
- MUST set HTTP response headers `Content-Type: text/event-stream`, `Cache-Control: no-cache`, and `Connection: keep-alive`.
- ALWAYS emit `emitNodeEnd` after `emitNodeStart` to allow client-side progress indicators to terminate cleanly.
- NEVER block stream emitter loops; perform intensive compute asynchronously in `np::WorkerPool`.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Browser receives tokens only after completion | HTTP response buffering enabled | Flush response chunks immediately after each token |
| Client UI disconnects during pauses | Idle connection timeout | Send periodic keep-alive heartbeat comments (`: ping\n\n`) |
| Unformatted text in browser | Single newline trailing SSE data | Use `msg.toSSE()` which appends `\n\n` |

## Verify
Verify event formatting and emitter subscription:
```bash
cl /std:c++17 /EHsc /Inovacpp assets/streaming_example.cpp
```

## Go deeper
- [`references/sse-protocol.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-streaming/references/sse-protocol.md): Detailed event taxonomy, HTTP headers, and browser reconnection strategies.


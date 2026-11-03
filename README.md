<div align="center">
  <img src="https://raw.githubusercontent.com/CfRadar/NovaCPP/main/src/assets/NovaCPP.png" alt="NovaCPP Framework" width="420" />
  
  <h1>NovaCPP</h1>
  
  <p>
    <a href="https://github.com/CfRadar/NovaCPP/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="NovaCPP license"></a>
    <a href="#"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build Status"></a>
    <a href="#"><img src="https://img.shields.io/badge/version-3.0.0-blue.svg" alt="NovaCPP version"></a>
    <a href="#"><img src="https://img.shields.io/badge/language-C%2B%2B17-blue.svg" alt="C++ version"></a>
    <a href="https://github.com/CfRadar/NovaCPP/pulls"><img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg" alt="PRs Welcome"></a>
  </p>
</div>

NovaCPP is a high-performance C++ web framework designed to build responsive, interactive user interfaces and resilient backend systems with **C++ execution speed** and **modern React-like developer ergonomics**.

* **Declarative & Component-Based:** Build encapsulated components in standard C++ that manage their own state and surgically update only changed DOM nodes.
* **Full-Stack Monolith (No-API):** Write UI layouts, business logic, and database operations in a single C++ executable with zero JSON serialization overhead.
* **Instant Fast Refresh HMR:** Incremental ~1s C++ recompilation pushed directly over Server-Sent Events without resetting application state.
* **Industry-Dominant npm Ecosystem:** Tap into npm's 2+ million browser packages directly from C++ using automated esbuild pre-bundling and declarative data attributes.
* **In-Framework AI Agent Skills:** Ships 22 canonical skills across 6 tiers, equipping Google Antigravity, Claude Code, Cursor, Copilot, and Windsurf to write 100% compiling, idiomatic C++17 on the first try.
* **Native Agentic AI Graph Engine:** Built-in Pregel cyclic execution (`StateGraph`), state reducers, time-travel checkpointing, real-time SSE streaming, and AVX2 SIMD vector memory.

---

## 30-Second Quickstart

Bootstrap a new project instantly using `npx create-novacpp`:

```bash
npx create-novacpp@latest my-app
cd my-app

# Launch the hot reloading development runner
npm run dev
# OR on Windows PowerShell
.\nova dev
```

Open [http://localhost:8080](http://localhost:8080) to experience the live animated starter showcase.

---

## In-Framework AI Agent Skills

NovaCPP includes native agent skill adapters generated directly from verified C++ headers:

```bash
# Auto-detect your editor and install skills
npx create-novacpp agent install

# Run health diagnostics on agent skills
npx create-novacpp agent doctor
```

### The First Prompt for Agents

Open your scaffolded project in Google Antigravity, Claude Code, Cursor, Copilot, or Windsurf and submit this prompt:

> *"Add a notes page with create and delete actions, stored in the SQLite database with a Bento Card UI and surgical DOM updates."*

Because the in-framework skills are active, your assistant immediately:
1. Allocates components with `static` lifetime satisfying Nova Component Lifetime Stack Guards.
2. Emits parameterized queries (`Database::execute(...)`) preventing SQL injection.
3. Uses `np::HtmlBuilder` with `nova-target` and `nova-click` for surgical DOM morphing.
4. Uses verified framework headers (`novacpp/html.hpp`, `novacpp/state.hpp`, `novacpp/sqlite.hpp`).

---

## Interactive Example

```cpp
#include "novacpp/html.hpp"
#include "novacpp/state.hpp"

// 1. Thread-safe session-isolated reactive state
np::State<int> counter(0);

void renderApp(np::NovaBuilder& np) {
    // 2. Direct C++ callback for button click
    np.onClick("increment", []() {
        counter = counter.get() + 1;
    });

    // 3. Declarative UI stream
    np << "<div class='bento-card col-4'>"
       << "    <h1>Counter: " + std::to_string(counter.get()) + "</h1>"
       << "    <button class='btn btn-primary' nova-click='increment'>+ Increment</button>"
       << "</div>";
}
```

---

## Documentation Sections

Explore the complete documentation on the [Nova Documentation Site](https://nova-documentation.vercel.app/):

### [Getting Started](https://nova-documentation.vercel.app/learn/getting-started/introduction)
* [Introduction & Philosophy](https://nova-documentation.vercel.app/learn/getting-started/introduction) — The "No-API" paradigm and core framework pillars.
* [Installation & Quickstart](https://nova-documentation.vercel.app/learn/getting-started/installation) — Prerequisites, CLI scaffolding, agent setup, and production builds.
* [CLI Project Scaffolder](https://nova-documentation.vercel.app/learn/getting-started/cli-scaffolder) — Templates, diagnostics, and `agent` CLI management.
* [Interactive Examples](https://nova-documentation.vercel.app/learn/getting-started/interactive-examples) — Walkthrough of the reactive counter, live clock, REST fetch, and npm confetti.
* [Troubleshooting & Gotchas](https://nova-documentation.vercel.app/learn/getting-started/troubleshooting-and-gotchas) — Top compiler diagnostics and stack guard invariants.

### [Describing the UI](https://nova-documentation.vercel.app/learn/describing-the-ui)
* [Your First Component](https://nova-documentation.vercel.app/learn/describing-the-ui/your-first-component) — Creating and inheriting `np::Component`.
* [Writing Code in C++](https://nova-documentation.vercel.app/learn/describing-the-ui/writing-code-in-cpp) — Dynamic HTML rendering using standard C++ logic.
* [Rendering Individual Components](https://nova-documentation.vercel.app/learn/describing-the-ui/rendering-individual-components) — Invoking `renderComponent()` and the Component Lifetime Invariant.
* [Frontend HTML Directives](https://nova-documentation.vercel.app/learn/describing-the-ui/frontend-html-directives) — Deep dive into `nova-link`, `nova-click`, `nova-target`, and `nova-poll`.
* [Styling & Bento Grid](https://nova-documentation.vercel.app/learn/describing-the-ui/styling-and-bento-grid) — Mathematical 12-column bento system and styling tokens.
* [Theming & 3D Code Flip](https://nova-documentation.vercel.app/learn/describing-the-ui/theming-and-animations) — Light/Dark theme tokens and 3D component inspection cards.
* [Using npm Packages](https://nova-documentation.vercel.app/learn/describing-the-ui/using-npm-packages) — Pre-bundling npm modules via `esbuild` and browser import maps.

### [Understanding the Backend](https://nova-documentation.vercel.app/learn/understanding-the-backend)
* [The Monolith Architecture](https://nova-documentation.vercel.app/learn/understanding-the-backend/the-monolith-architecture) — Shared memory space between views and business logic.
* [Routing & main.cpp](https://nova-documentation.vercel.app/learn/understanding-the-backend/routing-and-main) — Setting up SPA routes, REST endpoints, and server boot.
* [Server-Side State](https://nova-documentation.vercel.app/learn/understanding-the-backend/server-side-state) — Multi-session isolation via `nova_session` cookies.
* [Distributed Redis State](https://nova-documentation.vercel.app/learn/understanding-the-backend/distributed-redis-state) — Multi-server scaling with centralized state cache.
* [Database Integration](https://nova-documentation.vercel.app/learn/understanding-the-backend/database-integration) — SQLite WAL connection pooling and transactions.
* [Live Background Polling](https://nova-documentation.vercel.app/learn/understanding-the-backend/live-background-polling) — Pushless real-time data streaming without WebSockets.
* [Native HTTP Client (np::fetch)](https://nova-documentation.vercel.app/learn/understanding-the-backend/native-http-client) — Calling REST APIs with native dual HTTP engines.
* [Hot Reloading & Fast Refresh](https://nova-documentation.vercel.app/learn/understanding-the-backend/hot-reloading-and-hmr) — Incremental ~1s C++ compiler and SSE push stream.
* [Cloud Deployment Guide](https://nova-documentation.vercel.app/learn/understanding-the-backend/cloud-deployment-guide) — Docker multi-stage deployment to Railway, Render, and Fly.io.

### [Agentic AI Engine](https://nova-documentation.vercel.app/learn/agentic-ai)
* [Agent Skills & AI Instructions](https://nova-documentation.vercel.app/learn/agentic-ai/skills-and-instructions) — In-framework skills, CLI management, first prompt, and Docs MCP server.
* [Pregel Graph Engine](https://nova-documentation.vercel.app/learn/agentic-ai/pregel-graph-engine) — Multi-node cyclical execution and parallel supersteps.
* [State Reducers & Channels](https://nova-documentation.vercel.app/learn/agentic-ai/state-reducers-and-channels) — Append, overwrite, and custom reduction channels.
* [Ultra-Low Latency Tools & MCP](https://nova-documentation.vercel.app/learn/agentic-ai/tools-and-native-mcp) — Model Context Protocol stdio transport and schema generation.
* [WAL Checkpointing & Time-Travel](https://nova-documentation.vercel.app/learn/agentic-ai/wal-checkpointing-and-timetravel) — Step rewinding and state graph timeline forking.
* [Human-in-the-Loop Breakpoints](https://nova-documentation.vercel.app/learn/agentic-ai/human-in-the-loop-breakpoints) — Interrupting graph execution for approval before resumption.
* [Real-Time SSE Streaming](https://nova-documentation.vercel.app/learn/agentic-ai/real-time-sse-streaming) — Low-latency token streams and direct DOM hydration.
* [SIMD Vector Memory & Agentic RAG](https://nova-documentation.vercel.app/learn/agentic-ai/vector-memory-and-rag) — AVX2/SSE2 accelerated cosine similarity.
* [Enterprise Observability & Resilience](https://nova-documentation.vercel.app/learn/agentic-ai/telemetry-observability-resilience) — RAII span telemetry and Prometheus metrics.

### [API Reference](https://nova-documentation.vercel.app/learn/api-reference/novabuilder)
* [`np::NovaBuilder`](https://nova-documentation.vercel.app/learn/api-reference/novabuilder) — Route registration, event handlers, and server lifecycle.
* [`np::Component`](https://nova-documentation.vercel.app/learn/api-reference/component) — Base class, constructor ID binding, and virtual `render()`.
* [`np::State<T>`](https://nova-documentation.vercel.app/learn/api-reference/state) — Thread-safe reactive state variables and operators.
* [`np::fetch`](https://nova-documentation.vercel.app/learn/api-reference/fetch) — Native cross-platform HTTP client functions.
* [`np::StateGraph`](https://nova-documentation.vercel.app/learn/api-reference/stategraph) — Pregel cyclic workflow graph engine.
* [`np::Tools`](https://nova-documentation.vercel.app/learn/api-reference/tools-and-mcp) — Tool registration and MCP stdio server.
* [`np::VectorMemory`](https://nova-documentation.vercel.app/learn/api-reference/vector-memory-and-rag) — SIMD vector embeddings and cosine search.
* [`np::CheckpointManager`](https://nova-documentation.vercel.app/learn/api-reference/checkpoint-and-timetravel) — SQLite WAL graph state snapshots.

---

## Contributing

Development of NovaCPP happens in the open on GitHub. Pull requests, bug reports, and suggestions are warmly welcome!

---

## License

NovaCPP is [MIT licensed](LICENSE).

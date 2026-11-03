---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Durable Checkpoints & State Persistence

<!-- symbols: ICheckpointer, InMemoryCheckpointer, SQLiteCheckpointer, CheckpointMetadata, forkTimeline -->

## When to use
Use this skill when building multi-turn conversational agents, long-running workflows that survive server restarts, human-in-the-loop approval checkpoints, or time-travel history replay in `novacpp/checkpoint.hpp`.

## Prerequisites
- A NovaCPP project including `novacpp/nova.hpp` or `novacpp/checkpoint.hpp`.
- State structures must be copyable and default-constructible.

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <iostream>

struct FlowState {
    std::string threadId;
    int progress = 0;
};

void runCheckpointDemo() {
    auto checkpointer = std::make_shared<np::InMemoryCheckpointer<FlowState>>();

    FlowState state{"thread_001", 10};
    checkpointer->save("thread_001", 1, 0, "step_one", state, "{\"status\":\"ok\"}");

    FlowState restored;
    np::CheckpointMetadata meta;
    if (checkpointer->loadLatest("thread_001", restored, meta)) {
        std::cout << "Restored step from node: " << meta.nodeName << " with progress: " << restored.progress << std::endl;
    }
}
```
Full compilable source available in [`assets/checkpoint_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-checkpoints/assets/checkpoint_example.cpp).

## Steps
1. **Choose Checkpoint Engine**:
   - `InMemoryCheckpointer<T>`: Sub-microsecond execution, ideal for ephemeral sessions and test fixtures.
   - `SQLiteCheckpointer<T>`: Durable Write-Ahead Log (WAL) storage, persisting state across server restarts.
2. **Attach to Graph**:
   Configure the checkpointer directly on your `np::StateGraph`:
   ```cpp
   auto checkpointer = std::make_shared<np::SQLiteCheckpointer<FlowState>>("data/checkpoints.db");
   graph.setCheckpointer(checkpointer);
   ```
3. **Execute with Thread Identifier**:
   Invoke the compiled graph supplying a unique thread ID:
   ```cpp
   FlowState finalState = compiled.invoke(initialState, "session-abc-123");
   ```
4. **Time-Travel & Branching**:
   Fork execution timelines from a specific historical checkpoint ID:
   ```cpp
   checkpointer->forkTimeline("session-abc-123", 2, "session-abc-forked");
   ```
5. **Human-in-the-Loop Resumption**:
   Resume an interrupted graph after external human input:
   ```cpp
   FlowState updated = compiled.resume("session-abc-123", [](FlowState& s) {
       s.progress = 100;
   });
   ```

## Rules
- MUST provide a non-empty `threadId` when invoking graphs with durable checkpointers.
- MUST use `SQLiteCheckpointer` in production environments requiring persistence across container restarts.
- NEVER mutate past checkpoint entries directly; use `forkTimeline` to create alternative execution branches.
- State structs serialized by `SQLiteCheckpointer` MUST be standard-layout or provide serialization logic.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Checkpoints not recorded | Graph invoked with empty thread ID (`""`) | Pass explicit thread identifier: `compiled.invoke(state, "thread-id")` |
| `Cannot resume thread: No checkpointer configured` | `compiled.resume` called on uncheckpointed graph | Attach checkpointer with `graph.setCheckpointer(...)` before compile |
| Checkpoint database locked | Concurrent unpooled SQLite handles | Rely on Nova's built-in WAL mode and connection pooling |

## Verify
Verify checkpoint lifecycle and rollback logic:
```bash
cl /std:c++17 /EHsc /Inovacpp assets/checkpoint_example.cpp
```

## Go deeper
- [`references/time-travel.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-checkpoints/references/time-travel.md): Lineage tracking, parent-child DAG traversal, and state rollback mechanics.

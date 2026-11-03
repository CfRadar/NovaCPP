# NovaGraph Pregel Engine Architecture

## Cyclic Superstep Mechanics
NovaGraph implements the Pregel computational model designed for iterative graph-based AI agent logic:
1. **Discrete Supersteps**: Each superstep processes one or more active nodes concurrently.
2. **Deterministic Channel Propagation**: Mutated state updates are collected and merged across channel reducers.
3. **Synchronization Barrier**: Before the next superstep begins, all parallel node tasks synchronize, preventing data races.

## Edge Types
- **Static Edges**: Unconditional directed transitions (`addEdge("planner", "executor")`).
- **Conditional Edges**: Dynamic routing lambdas evaluating the current state and returning the next node key:
  ```cpp
  graph.addConditionalEdges("evaluator", [](const AgentState& s) {
      if (s.confidence > 0.9) return np::END;
      return "planner";
  }, { {"planner", "planner"}, {np::END, np::END} });
  ```

## Concurrency
Parallel node tasks run across `np::WorkerPool` worker threads without thread allocation overhead.


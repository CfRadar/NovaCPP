# Time-Travel & Lineage Architecture

## Lineage Model
Every checkpoint in NovaGraph captures:
- `threadId`: Conversation or task partition key.
- `checkpointId`: Monotonically increasing step sequence number.
- `parentId`: Preceding step sequence number, forming an immutable directed acyclic graph.
- `nodeName`: Active node producing the state transition.
- `metadata`: JSON payload containing interruption flags, human reviewer feedback, or latency stamps.

## Timeline Forking
When branching an agent execution (e.g. exploring multiple tool choices or what-if scenarios), `forkTimeline` clones checkpoints up to target step `N` under a new `threadId`. Subsequent supersteps on the new thread diverge without affecting the original thread's history.


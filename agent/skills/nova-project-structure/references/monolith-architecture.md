# NovaCPP Monolith Architecture Reference

## Why the Monolithic Architecture Matters

Traditional modern web development forces a strict division between a JavaScript frontend (Next.js, React, Vue) and a backend service (Node, Go, Python) communicating over JSON REST or GraphQL.

NovaCPP merges these concerns into a **Full-Stack Monolith**:
1. **Zero Serialization Overhead**: Views and business logic share the same C++ process memory. Passing a database record into a component requires no JSON stringification or deserialization.
2. **Sub-Millisecond Response Latencies**: Native execution avoids the V8 JavaScript garbage collector pauses and Python interpreter GIL bottlenecks.
3. **Single Binary Deployment**: The entire application compiles into a single executable (`NovaCPP.exe` or `NovaCPP`), simplifying Docker containers and Kubernetes deployments.
4. **Reactive Surgical Updates**: When an action is dispatched, the server re-renders only the targeted component sub-tree (`X-Nova-Target`), providing React-like client reactivity with native backend speed.


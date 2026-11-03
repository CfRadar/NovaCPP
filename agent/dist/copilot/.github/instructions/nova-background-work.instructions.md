---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Asynchronous Worker Pool & Background Tasks

<!-- symbols: WorkerPool, defaultPool, enqueue -->

## When to use
Use this skill when offloading CPU-intensive processing, executing long-running file exports, sending background emails, or ensuring that user-facing HTTP request threads return immediately without stalling.

## Prerequisites
- Include header `novacpp/worker_pool.hpp` or `novacpp/nova.hpp`.

## Quick start
```cpp
#include "novacpp/nova.hpp"

void handleLongComputation() {
    // Enqueue task to default worker pool
    np::WorkerPool::defaultPool().enqueue([]() {
        // Runs on background worker thread
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Background task completed!" << std::endl;
    });
}
```
Full compilable source available in [`assets/worker_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-background-work/assets/worker_example.cpp).

## Steps
1. **Access the Global Thread Pool**:
   NovaCPP provides an optimized global pool sized to `std::thread::hardware_concurrency()`:
   ```cpp
   auto& pool = np::WorkerPool::defaultPool();
   ```
2. **Dispatch Non-Blocking Tasks**:
   Call `pool.enqueue(lambda)` inside event listeners, form handlers, or route callbacks:
   ```cpp
   np.onClick("generate_report", []() {
       np::WorkerPool::defaultPool().enqueue([]() {
           processLargeReport();
       });
   });
   ```
3. **Task Return Value with `std::future`**:
   `enqueue` returns a `std::future<T>` enabling asynchronous result retrieval:
   ```cpp
   std::future<int> futureResult = np::WorkerPool::defaultPool().enqueue([]() -> int {
       return calculateComplexMetric();
   });
   // Later or in another thread:
   int value = futureResult.get();
   ```
4. **Inspect Pool Metrics**:
   Monitor pool size and capacity:
   ```cpp
   size_t activeWorkers = np::WorkerPool::defaultPool().size();
   ```
5. **Critical Capture Invariant**:
   Never capture `httplib::Request&` or `Response&` by reference in an asynchronous lambda:
   ```cpp
   // DANGEROUS:
   // np.route("/bad", [](Request& req, Response& res) {
   //     WorkerPool::defaultPool().enqueue([&req]() { use(req.body); }); // Access violation!
   // });

   // CORRECT:
   np.route("/good", [](np::RenderContext& np) {
       std::string payloadCopy = "user-data"; // Copy by value
       np::WorkerPool::defaultPool().enqueue([payloadCopy]() {
           use(payloadCopy); // Safe!
       });
   });
   ```

## Rules
- MUST capture request parameters and body by value when dispatching to the worker pool.
- MUST offload any operation exceeding ~50ms to the worker pool to maintain high server throughput.
- NEVER capture `httplib::Request&` or `httplib::Response&` by reference into asynchronous background lambdas.
- NEVER call blocking `.get()` on a `std::future` directly within the HTTP request thread, as this defeats asynchronous offloading.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Random crash inside background thread | Lambda captured temporary stack variable by reference (`[&]`) | Capture values explicitly by copy (`[val]`) |
| Server throughput degrades under load | Long-running queries blocking the HTTP worker thread | Move computation to `np::WorkerPool::defaultPool().enqueue(...)` |
| Tasks not executed on shutdown | Application exited before queue drained | WorkerPool destructor automatically joins active workers on server stop |

## Verify
Execute the worker pool stress test:
```bash
nova.bat test
```
Validates 1,000 background tasks dispatched in under 15ms via `simulate_worker_pool.exe`.

## Go deeper
- [`references/worker-pool.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-background-work/references/worker-pool.md): Queue synchronization, condition variable signaling, and bounded backpressure.

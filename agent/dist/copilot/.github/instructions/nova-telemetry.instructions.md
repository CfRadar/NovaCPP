---
applyTo: "**/*.cpp,**/*.hpp"
---

# NovaCPP Telemetry, Prometheus Metrics & Profiling

<!-- symbols: Telemetry, TraceSpan, GraphInspector, exportPrometheusMetrics -->

## When to use
Use this skill when exposing `/metrics` endpoints for Prometheus/Grafana scrapers, profiling node latency with microsecond precision, tracking total supersteps, or embedding live Mermaid visualizers in `novacpp/telemetry.hpp`.

## Prerequisites
- A NovaCPP project including `novacpp/nova.hpp` or `novacpp/telemetry.hpp`.

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <iostream>

void recordMetrics() {
    {
        np::TraceSpan span("llm_reasoning_node");
        np::Telemetry::recordSuperStep();
        np::Telemetry::recordTokens(256);
        np::Telemetry::recordNodeSuccess();
    } // TraceSpan automatically finishes on scope exit and records microseconds

    std::string prometheus = np::Telemetry::exportPrometheusMetrics();
    std::cout << prometheus;
}
```
Full compilable source available in [`assets/telemetry_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-telemetry/assets/telemetry_example.cpp).

## Steps
1. **Instrument Graph Nodes with TraceSpan**:
   Wrap execution blocks with `np::TraceSpan` for RAII microsecond measurement:
   ```cpp
   {
       np::TraceSpan span("database_query");
       // execute heavy work
   }
   ```
2. **Record Event Counters**:
   Increment atomic counters for supersteps, tokens, checkpoints, and errors:
   ```cpp
   np::Telemetry::recordSuperStep();
   np::Telemetry::recordTokens(count);
   np::Telemetry::recordCheckpoint();
   ```
3. **Expose Prometheus Metrics Endpoint**:
   Register a raw GET route returning standard text exposition format:
   ```cpp
   app.route("/metrics", [](np::RenderContext& np) {
       np.res.set_content(np::Telemetry::exportPrometheusMetrics(), "text/plain; version=0.0.4; charset=utf-8");
   });
   ```
4. **Embed Visual Graph Inspector**:
   Render the Mermaid DAG into interactive Bento cards:
   ```cpp
   std::string cardHtml = np::GraphInspector::renderCard(compiledGraph.toMermaid());
   ```

## Rules
- MUST expose `/metrics` using `text/plain; version=0.0.4; charset=utf-8` header.
- MUST use `TraceSpan` RAII scopes rather than manual start/stop timestamps.
- NEVER block metrics gathering with mutex locks; `np::Telemetry` uses atomic variables (`std::atomic<uint64_t>`).
- Prometheus counters MUST be monotonic; do not reset counters in production without an explicit Prometheus restart.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Prometheus scraper reports syntax error | Missing trailing newline in metrics string | Use `np::Telemetry::exportPrometheusMetrics()` which formats correctly |
| Microsecond durations remain 0 | `TraceSpan` destroyed before work starts | Scope `TraceSpan` around the entire function or block |
| Race conditions in custom counters | Using raw integer increments across threads | Use `std::atomic` or `np::Telemetry` helper methods |

## Verify
Verify metric aggregation and Prometheus text generation:
```bash
cl /std:c++17 /EHsc /Inovacpp assets/telemetry_example.cpp
```

## Go deeper
- [`references/prometheus-metrics.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-telemetry/references/prometheus-metrics.md): Metric dictionary, Grafana dashboard queries, and Prometheus scraping configuration.

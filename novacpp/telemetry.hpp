#pragma once

#include <string>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <sstream>
#include <memory>
#include <functional>

namespace np {

// Centralized thread-safe Prometheus metrics engine for NovaGraph
class Telemetry {
public:
    static inline std::atomic<uint64_t> totalSuperSteps{0};
    static inline std::atomic<uint64_t> totalNodeExecutions{0};
    static inline std::atomic<uint64_t> totalFailedNodes{0};
    static inline std::atomic<uint64_t> totalCheckpointsWritten{0};
    static inline std::atomic<uint64_t> totalTokensStreamed{0};
    static inline std::atomic<uint64_t> totalExecutionMicroseconds{0};

    static void recordSuperStep(uint64_t count = 1) {
        totalSuperSteps.fetch_add(count, std::memory_order_relaxed);
    }

    static void recordNodeSuccess(uint64_t count = 1) {
        totalNodeExecutions.fetch_add(count, std::memory_order_relaxed);
    }

    static void recordNodeFailure(uint64_t count = 1) {
        totalFailedNodes.fetch_add(count, std::memory_order_relaxed);
    }

    static void recordCheckpoint(uint64_t count = 1) {
        totalCheckpointsWritten.fetch_add(count, std::memory_order_relaxed);
    }

    static void recordTokens(uint64_t count = 1) {
        totalTokensStreamed.fetch_add(count, std::memory_order_relaxed);
    }

    static void recordExecutionDuration(uint64_t us) {
        totalExecutionMicroseconds.fetch_add(us, std::memory_order_relaxed);
    }

    static void reset() {
        totalSuperSteps.store(0, std::memory_order_relaxed);
        totalNodeExecutions.store(0, std::memory_order_relaxed);
        totalFailedNodes.store(0, std::memory_order_relaxed);
        totalCheckpointsWritten.store(0, std::memory_order_relaxed);
        totalTokensStreamed.store(0, std::memory_order_relaxed);
        totalExecutionMicroseconds.store(0, std::memory_order_relaxed);
    }

    // Export metrics in standard Prometheus wire format
    static std::string exportPrometheusMetrics() {
        std::ostringstream ss;
        ss << "# HELP novagraph_supersteps_total Total completed graph super-steps\n"
           << "# TYPE novagraph_supersteps_total counter\n"
           << "novagraph_supersteps_total " << totalSuperSteps.load() << "\n\n"

           << "# HELP novagraph_node_executions_total Total executed agent nodes\n"
           << "# TYPE novagraph_node_executions_total counter\n"
           << "novagraph_node_executions_total " << totalNodeExecutions.load() << "\n\n"

           << "# HELP novagraph_node_failures_total Total failed node transitions\n"
           << "# TYPE novagraph_node_failures_total counter\n"
           << "novagraph_node_failures_total " << totalFailedNodes.load() << "\n\n"

           << "# HELP novagraph_checkpoints_total Total persisted WAL snapshots\n"
           << "# TYPE novagraph_checkpoints_total counter\n"
           << "novagraph_checkpoints_total " << totalCheckpointsWritten.load() << "\n\n"

           << "# HELP novagraph_tokens_streamed_total Total streamed LLM tokens\n"
           << "# TYPE novagraph_tokens_streamed_total counter\n"
           << "novagraph_tokens_streamed_total " << totalTokensStreamed.load() << "\n\n"

           << "# HELP novagraph_execution_microseconds_total Total execution time across all nodes in microseconds\n"
           << "# TYPE novagraph_execution_microseconds_total counter\n"
           << "novagraph_execution_microseconds_total " << totalExecutionMicroseconds.load() << "\n";

        return ss.str();
    }
};

// RAII Trace Span for zero-overhead microsecond duration profiling
class TraceSpan {
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
    bool m_finished = false;

public:
    explicit TraceSpan(std::string name)
        : m_name(std::move(name)), m_start(std::chrono::high_resolution_clock::now()) {}

    ~TraceSpan() {
        if (!m_finished) {
            finish();
        }
    }

    TraceSpan(const TraceSpan&) = delete;
    TraceSpan& operator=(const TraceSpan&) = delete;

    TraceSpan(TraceSpan&& other) noexcept
        : m_name(std::move(other.m_name)), m_start(other.m_start), m_finished(other.m_finished) {
        other.m_finished = true;
    }

    uint64_t elapsedMicroseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count());
    }

    uint64_t finish() {
        if (m_finished) return 0;
        m_finished = true;
        uint64_t us = elapsedMicroseconds();
        Telemetry::recordExecutionDuration(us);
        return us;
    }

    const std::string& name() const {
        return m_name;
    }
};

// Visual DAG Inspector HTML Renderer
class GraphInspector {
public:
    static std::string renderCard(const std::string& mermaidDAG, const std::string& title = "Active Agent Execution Topology") {
        std::ostringstream ss;
        ss << "<div class='bento-card col-12' id='graph-inspector-card'>\n"
           << "  <div class='card-header'>\n"
           << "    <span class='badge badge-cyan'>GRAPH VISUALIZER</span>\n"
           << "    <span class='badge badge-green'>SUPER-STEP ENGINE ACTIVE</span>\n"
           << "  </div>\n"
           << "  <h3>" << title << "</h3>\n"
           << "  <div class='mermaid-dag-container'>\n"
           << "    <pre class='mermaid'>\n"
           << mermaidDAG
           << "    </pre>\n"
           << "  </div>\n"
           << "</div>";
        return ss.str();
    }
};

// Execution budget and timeout control for runaway loop prevention
struct ExecutionBudget {
    size_t maxRecursion = 50;
    std::chrono::milliseconds timeoutMs{0}; // 0 = unlimited

    ExecutionBudget(size_t recursionLimit = 50, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
        : maxRecursion(recursionLimit), timeoutMs(timeout) {}

    bool hasTimeout() const {
        return timeoutMs.count() > 0;
    }

    bool isExpired(const std::chrono::high_resolution_clock::time_point& startTime) const {
        if (!hasTimeout()) return false;
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        return elapsed >= timeoutMs;
    }
};

} // namespace np

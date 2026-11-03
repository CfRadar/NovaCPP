#include "novacpp/nova.hpp"
#include <iostream>
#include <thread>
#include <chrono>

void demonstrateTelemetry() {
    // 1. Reset counters for local session
    np::Telemetry::reset();

    // 2. Profile work using RAII TraceSpan
    {
        np::TraceSpan span("vector_embedding_retrieval");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        np::Telemetry::recordSuperStep(1);
        np::Telemetry::recordNodeSuccess(1);
        np::Telemetry::recordTokens(512);
        np::Telemetry::recordCheckpoint(1);
    }

    // 3. Export Prometheus metrics payload
    std::string metrics = np::Telemetry::exportPrometheusMetrics();
    std::cout << "--- PROMETHEUS METRICS EXPOSITION ---\n" << metrics;

    // 4. Generate Mermaid Inspector Card HTML
    std::string mermaid = "graph TD\n  START([START]) --> plan\n  plan --> execute\n  execute --> END([END])";
    std::string card = np::GraphInspector::renderCard(mermaid, "Test Pipeline");
    std::cout << "\n--- GRAPH INSPECTOR CARD HTML (Length: " << card.size() << ") ---\n";
}

int main() {
    demonstrateTelemetry();
    return 0;
}


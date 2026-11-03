#include "novacpp/nova.hpp"
#include <iostream>

struct PipelineState {
    std::string topic;
    int stepCount = 0;
    std::string logOutput;
};

void runWorkflow() {
    np::StateGraph<PipelineState> graph;

    graph.addNode("plan", [](const PipelineState& in, PipelineState& out) {
        out = in;
        out.stepCount++;
        out.logOutput += "[Step " + std::to_string(out.stepCount) + "] Planned for: " + out.topic + "\n";
    });

    graph.addNode("execute", [](const PipelineState& in, PipelineState& out) {
        out = in;
        out.stepCount++;
        out.logOutput += "[Step " + std::to_string(out.stepCount) + "] Executed.\n";
    });

    graph.setEntryPoint("plan");
    graph.addEdge("plan", "execute");

    graph.addConditionalEdges("execute", [](const PipelineState& s) {
        return (s.stepCount >= 4) ? np::END : "plan";
    }, { {"plan", "plan"}, {np::END, np::END} });

    auto compiled = graph.compile();

    PipelineState initial{"Distributed Web Architecture", 0, ""};
    PipelineState result = compiled.invoke(initial);

    std::cout << "Execution completed in " << result.stepCount << " steps:\n" << result.logOutput;
}

int main() {
    runWorkflow();
    return 0;
}


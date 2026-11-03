#include "novacpp/nova.hpp"
#include <iostream>

struct WorkflowData {
    std::string task;
    int completedSteps = 0;
};

void demonstrateCheckpoints() {
    auto checkpointer = std::make_shared<np::InMemoryCheckpointer<WorkflowData>>();

    std::string primaryThread = "agent-run-101";

    // 1. Record sequence of checkpoints
    WorkflowData s1{"Analyze system", 1};
    checkpointer->save(primaryThread, 1, 0, "analyzer", s1, "{\"status\":\"ok\"}");

    WorkflowData s2{"Optimize queries", 2};
    checkpointer->save(primaryThread, 2, 1, "optimizer", s2, "{\"status\":\"ok\"}");

    // 2. Load latest checkpoint
    WorkflowData latest;
    np::CheckpointMetadata meta;
    if (checkpointer->loadLatest(primaryThread, latest, meta)) {
        std::cout << "Latest step: " << meta.checkpointId << " at node: " << meta.nodeName
                  << " Task: " << latest.task << std::endl;
    }

    // 3. Inspect history
    auto history = checkpointer->getHistory(primaryThread);
    std::cout << "History length for " << primaryThread << ": " << history.size() << std::endl;

    // 4. Fork timeline to new branch
    std::string forkedThread = "agent-run-101-fork";
    checkpointer->forkTimeline(primaryThread, 1, forkedThread);
    std::cout << "Forked timeline to: " << forkedThread << std::endl;

    WorkflowData forkedLatest;
    np::CheckpointMetadata forkedMeta;
    if (checkpointer->loadLatest(forkedThread, forkedLatest, forkedMeta)) {
        std::cout << "Forked latest step: " << forkedMeta.checkpointId << " Node: " << forkedMeta.nodeName << std::endl;
    }
}

int main() {
    demonstrateCheckpoints();
    return 0;
}


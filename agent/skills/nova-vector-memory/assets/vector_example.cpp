#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateVectorMemory() {
    // 1. Initialize 4-dimensional vector index
    np::VectorIndex index(4);

    // 2. Add semantic knowledge vectors
    index.addDocument("doc_1", "NovaCPP full stack high performance framework", {1.0f, 0.0f, 0.0f, 0.0f});
    index.addDocument("doc_2", "Reactive Bento grid responsive layouts", {0.0f, 1.0f, 0.0f, 0.0f});
    index.addDocument("doc_3", "Distributed Redis session caching cluster", {0.0f, 0.0f, 1.0f, 0.0f});
    index.addDocument("doc_4", "Pregel state graphs for autonomous agents", {0.0f, 0.0f, 0.0f, 1.0f});

    std::cout << "Indexed documents: " << index.size() << std::endl;

    // 3. Search using query embedding closely aligned with document 4
    std::vector<float> queryVector = {0.1f, 0.0f, 0.2f, 0.95f};
    auto results = index.search(queryVector, 2, 0.5f);

    std::cout << "Top matches for agent query:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Rank #" << (i + 1) << " [Score: " << results[i].first << "] "
                  << results[i].second.id << " -> " << results[i].second.text << std::endl;
    }
}

int main() {
    demonstrateVectorMemory();
    return 0;
}


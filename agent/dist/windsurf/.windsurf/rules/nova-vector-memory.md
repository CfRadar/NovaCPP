# NovaCPP Rule: nova-vector-memory

# NovaCPP SIMD Vector Memory & Agentic RAG

<!-- symbols: VectorIndex, VectorDocument, cosineSimilaritySIMD, search, addDocument -->

## When to use
Use this skill when building Retrieval-Augmented Generation (RAG) pipelines, indexing unstructured knowledge documents, querying embeddings with sub-millisecond AVX2/SSE2 cosine similarity, or managing agent long-term memory in `novacpp/simd_vector.hpp`.

## Prerequisites
- A NovaCPP project including `novacpp/nova.hpp` or `novacpp/simd_vector.hpp`.
- Vector embeddings generated via local or remote embedding models (e.g. 384-d, 768-d, or 1536-d float arrays).

## Quick start
```cpp
#include "novacpp/nova.hpp"
#include <iostream>

void demonstrateRAG() {
    np::VectorIndex index(4); // 4-dimensional example index

    index.addDocument("doc_1", "NovaCPP offers zero-copy C++ performance", {1.0f, 0.0f, 0.0f, 0.0f});
    index.addDocument("doc_2", "SQLite WAL handles high-concurrency workloads", {0.0f, 1.0f, 0.0f, 0.0f});

    std::vector<float> query = {0.9f, 0.1f, 0.0f, 0.0f};
    auto matches = index.search(query, 1);

    if (!matches.empty()) {
        std::cout << "Top match: " << matches[0].second.text 
                  << " (Cosine Score: " << matches[0].first << ")\n";
    }
}
```
Full compilable source available in [`assets/vector_example.cpp`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-vector-memory/assets/vector_example.cpp).

## Steps
1. **Initialize VectorIndex**:
   Configure index dimensionality matching your embedding model (e.g., 1536 for OpenAI `text-embedding-3-small`, 384 for `all-MiniLM-L6-v2`):
   ```cpp
   np::VectorIndex index(1536);
   ```
2. **Index Documents**:
   Add documents with unique ID, text content, embedding float vector, and metadata:
   ```cpp
   index.addDocument("doc_notes_42", "Release notes for v2.1", embeddingVector, {{"author", "admin"}});
   ```
3. **Execute SIMD Cosine Similarity Search**:
   Retrieve top-K matching documents:
   ```cpp
   auto results = index.search(queryEmbedding, 5, 0.7f); // top 5 with minimum cosine similarity 0.7
   for (const auto& [score, doc] : results) {
       std::cout << "Score " << score << ": " << doc.text << std::endl;
   }
   ```
4. **Hardware SIMD Acceleration**:
   Nova automatically uses AVX2 (processing 8 floats per instruction) or SSE2 (4 floats per instruction) when compiling with MSVC `/arch:AVX2` or GCC `-mavx2`.

## Rules
- MUST normalize query and document vector dimensions; dimension mismatches truncate to minimum dimension.
- MUST use `np::VectorIndex` for thread-safe concurrent reads and writes.
- ALWAYS index embeddings in background tasks (`np::WorkerPool`) when processing large batches.
- NEVER load multi-gigabyte vector indexes directly on the web server stack; use heap allocation.

## Common mistakes
| Symptom | Cause | Fix |
|---|---|---|
| Cosine similarity scores return 0.0 | Empty embedding vectors or zero magnitude | Ensure vectors contain non-zero float values |
| Low similarity between semantically identical texts | Unnormalized vectors or mismatched embedding models | Use consistent embedding model for both ingestion and search |
| Index lost on server restart | In-memory index used without persistence | Persist vectors into SQLite BLOB columns or load on startup |

## Verify
Verify SIMD cosine calculation and top-k nearest neighbor ranking:
```bash
cl /std:c++17 /EHsc /Inovacpp assets/vector_example.cpp
```

## Go deeper
- [`references/simd-rag.md`](file:///d:/ALL_USER_DATA/Nova/NovaCPP/agent/skills/nova-vector-memory/references/simd-rag.md): AVX2 register packing, cache alignment, and Agentic RAG prompt synthesis.

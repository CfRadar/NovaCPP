# SIMD Hardware Vector Acceleration & Agentic RAG

## Instruction Set Support
NovaCPP detects target CPU features during compilation:
- **AVX2 (`__AVX2__`)**: Processes 256-bit registers (8 single-precision floats) with fused multiply-accumulate (`_mm256_fmadd_ps`).
- **SSE2 (`_M_X64` / `__SSE2__`)**: Baseline on all 64-bit AMD and Intel processors (128-bit registers, 4 floats per instruction).
- **Scalar Fallback**: Clean portable C++ loops when SIMD flags are unavailable.

## Agentic RAG Pipeline
1. Ingest documents into SQLite with raw text and embedded float vectors.
2. In-memory `VectorIndex` holds active semantic chunks.
3. On incoming user question, compute query embedding and invoke `index.search(queryVec, 3)`.
4. Inject retrieved chunks into graph state for prompt synthesis.


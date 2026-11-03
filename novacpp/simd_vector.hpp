#pragma once

#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <iostream>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

#include "sqlite.hpp"

namespace np {

// Compute cosine similarity between two float vectors using SIMD hardware acceleration
inline float cosineSimilaritySIMD(const float* a, const float* b, size_t dim) {
    if (!a || !b || dim == 0) return 0.0f;
    float dot = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

#if defined(__AVX2__)
    // 256-bit AVX2 registers process 8 single-precision floats per instruction
    __m256 vDot = _mm256_setzero_ps();
    __m256 vNormA = _mm256_setzero_ps();
    __m256 vNormB = _mm256_setzero_ps();

    size_t i = 0;
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);

        vDot = _mm256_fmadd_ps(va, vb, vDot);
        vNormA = _mm256_fmadd_ps(va, va, vNormA);
        vNormB = _mm256_fmadd_ps(vb, vb, vNormB);
    }

    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, vDot);
    for (int j = 0; j < 8; ++j) dot += buffer[j];

    _mm256_storeu_ps(buffer, vNormA);
    for (int j = 0; j < 8; ++j) normA += buffer[j];

    _mm256_storeu_ps(buffer, vNormB);
    for (int j = 0; j < 8; ++j) normB += buffer[j];

    for (; i < dim; ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
#elif defined(_M_X64) || defined(__SSE2__)
    // 128-bit SSE registers process 4 single-precision floats per instruction (supported on all x86_64)
    __m128 vDot = _mm_setzero_ps();
    __m128 vNormA = _mm_setzero_ps();
    __m128 vNormB = _mm_setzero_ps();

    size_t i = 0;
    for (; i + 4 <= dim; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);

        vDot = _mm_add_ps(vDot, _mm_mul_ps(va, vb));
        vNormA = _mm_add_ps(vNormA, _mm_mul_ps(va, va));
        vNormB = _mm_add_ps(vNormB, _mm_mul_ps(vb, vb));
    }

    alignas(16) float buffer[4];
    _mm_storeu_ps(buffer, vDot);
    for (int j = 0; j < 4; ++j) dot += buffer[j];

    _mm_storeu_ps(buffer, vNormA);
    for (int j = 0; j < 4; ++j) normA += buffer[j];

    _mm_storeu_ps(buffer, vNormB);
    for (int j = 0; j < 4; ++j) normB += buffer[j];

    for (; i < dim; ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
#else
    for (size_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
#endif

    if (normA <= 0.0f || normB <= 0.0f) return 0.0f;
    return dot / (std::sqrt(normA) * std::sqrt(normB));
}

// Compute Euclidean (L2) distance between two float vectors using SIMD
inline float l2DistanceSIMD(const float* a, const float* b, size_t dim) {
    if (!a || !b || dim == 0) return 0.0f;
    float sumSq = 0.0f;

#if defined(__AVX2__)
    __m256 vSum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        vSum = _mm256_fmadd_ps(diff, diff, vSum);
    }
    alignas(32) float buffer[8];
    _mm256_storeu_ps(buffer, vSum);
    for (int j = 0; j < 8; ++j) sumSq += buffer[j];

    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        sumSq += diff * diff;
    }
#elif defined(_M_X64) || defined(__SSE2__)
    __m128 vSum = _mm_setzero_ps();
    size_t i = 0;
    for (; i + 4 <= dim; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 diff = _mm_sub_ps(va, vb);
        vSum = _mm_add_ps(vSum, _mm_mul_ps(diff, diff));
    }
    alignas(16) float buffer[4];
    _mm_storeu_ps(buffer, vSum);
    for (int j = 0; j < 4; ++j) sumSq += buffer[j];

    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        sumSq += diff * diff;
    }
#else
    for (size_t i = 0; i < dim; ++i) {
        float diff = a[i] - b[i];
        sumSq += diff * diff;
    }
#endif
    return std::sqrt(sumSq);
}

// Document record with text content, embedding vector, and associative metadata
struct VectorDocument {
    std::string id;
    std::string text;
    std::vector<float> embedding;
    std::unordered_map<std::string, std::string> metadata;

    VectorDocument() = default;

    VectorDocument(std::string docId, std::string content, std::vector<float> vec,
                   std::unordered_map<std::string, std::string> meta = {})
        : id(std::move(docId)), text(std::move(content)), embedding(std::move(vec)), metadata(std::move(meta)) {}
};

// Thread-safe high-performance in-process vector index
class VectorIndex {
private:
    std::vector<VectorDocument> m_documents;
    size_t m_dimension = 1536;
    mutable std::mutex m_mtx;

public:
    explicit VectorIndex(size_t dim = 1536) : m_dimension(dim) {}

    size_t dimension() const {
        return m_dimension;
    }

    void setDimension(size_t dim) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_dimension = dim;
    }

    void insert(VectorDocument doc) {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto& existing : m_documents) {
            if (existing.id == doc.id) {
                existing = std::move(doc);
                return;
            }
        }
        m_documents.push_back(std::move(doc));
    }

    void insert(std::string id, std::string text, std::vector<float> embedding,
                std::unordered_map<std::string, std::string> meta = {}) {
        insert(VectorDocument(std::move(id), std::move(text), std::move(embedding), std::move(meta)));
    }

    bool remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
            if (it->id == id) {
                m_documents.erase(it);
                return true;
            }
        }
        return false;
    }

    bool get(const std::string& id, VectorDocument& outDoc) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (const auto& doc : m_documents) {
            if (doc.id == id) {
                outDoc = doc;
                return true;
            }
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_documents.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_documents.size();
    }

    // Search top-K documents ranked by cosine similarity
    std::vector<std::pair<float, VectorDocument>> search(
        const std::vector<float>& queryEmbedding,
        size_t topK = 3,
        float minScore = -1.0f
    ) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_documents.empty()) return {};

        std::vector<std::pair<float, const VectorDocument*>> scored;
        scored.reserve(m_documents.size());

        for (const auto& doc : m_documents) {
            size_t evalDim = std::min({m_dimension, queryEmbedding.size(), doc.embedding.size()});
            float score = cosineSimilaritySIMD(queryEmbedding.data(), doc.embedding.data(), evalDim);
            if (score >= minScore) {
                scored.push_back({score, &doc});
            }
        }

        size_t k = std::min(topK, scored.size());
        std::partial_sort(scored.begin(), scored.begin() + k, scored.end(),
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            }
        );

        std::vector<std::pair<float, VectorDocument>> results;
        results.reserve(k);
        for (size_t i = 0; i < k; ++i) {
            results.push_back({scored[i].first, *scored[i].second});
        }
        return results;
    }

    // Export all documents for persistence or clustering
    std::vector<VectorDocument> getAllDocuments() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_documents;
    }
};

// Persistent vector store backed by SQLite WAL database
class SQLiteVectorStore {
private:
    std::shared_ptr<SQLiteConnection> m_conn;
    mutable std::mutex m_mtx;

    static std::string serializeMetadata(const std::unordered_map<std::string, std::string>& meta) {
        std::ostringstream ss;
        bool first = true;
        for (const auto& [k, v] : meta) {
            if (!first) ss << ";;";
            ss << k << "::" << v;
            first = false;
        }
        return ss.str();
    }

    static std::unordered_map<std::string, std::string> deserializeMetadata(const std::string& str) {
        std::unordered_map<std::string, std::string> meta;
        if (str.empty()) return meta;
        std::istringstream ss(str);
        std::string pair;
        while (std::getline(ss, pair, ';')) {
            if (pair.empty()) continue;
            auto delim = pair.find("::");
            if (delim != std::string::npos) {
                meta[pair.substr(0, delim)] = pair.substr(delim + 2);
            }
        }
        return meta;
    }

public:
    explicit SQLiteVectorStore(std::shared_ptr<SQLiteConnection> conn) : m_conn(std::move(conn)) {
        initializeTables();
    }

    explicit SQLiteVectorStore(const std::string& dbPath = ":memory:")
        : SQLiteVectorStore(std::make_shared<SQLiteConnection>(dbPath)) {}

    void initializeTables() {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_conn->execute(
            "CREATE TABLE IF NOT EXISTS nova_vector_store ("
            "  id TEXT PRIMARY KEY,"
            "  text TEXT NOT NULL,"
            "  dimension INTEGER NOT NULL,"
            "  embedding BLOB NOT NULL,"
            "  metadata TEXT"
            ");"
        );
    }

    void save(const VectorDocument& doc) {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::string blob(reinterpret_cast<const char*>(doc.embedding.data()),
                         doc.embedding.size() * sizeof(float));
        std::string metaStr = serializeMetadata(doc.metadata);

        m_conn->execute(
            "INSERT OR REPLACE INTO nova_vector_store (id, text, dimension, embedding, metadata) "
            "VALUES (?, ?, ?, ?, ?);",
            doc.id, doc.text, static_cast<int>(doc.embedding.size()), SQLiteBlob{blob}, metaStr
        );
    }

    bool remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_conn->execute("DELETE FROM nova_vector_store WHERE id = ?;", id);
        return true;
    }

    void loadIntoIndex(VectorIndex& index) {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto rows = m_conn->queryAll("SELECT id, text, dimension, embedding, metadata FROM nova_vector_store;");
        for (const auto& r : rows) {
            std::string id = r.at("id");
            std::string text = r.at("text");
            size_t dim = static_cast<size_t>(std::stoi(r.at("dimension")));
            const std::string& blob = r.at("embedding");

            std::vector<float> embedding(dim, 0.0f);
            if (blob.size() >= dim * sizeof(float)) {
                std::memcpy(embedding.data(), blob.data(), dim * sizeof(float));
            }

            auto meta = deserializeMetadata(r.at("metadata"));
            index.insert(VectorDocument(std::move(id), std::move(text), std::move(embedding), std::move(meta)));
        }
    }

    void syncFromIndex(const VectorIndex& index) {
        auto docs = index.getAllDocuments();
        for (const auto& doc : docs) {
            save(doc);
        }
    }
};

} // namespace np


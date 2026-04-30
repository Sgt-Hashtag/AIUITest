#pragma once
#include <string>
#include <vector>
#include "llama.h"

class LlamaEngine {
public:
    static LlamaEngine& instance();
    
    // Load an EMBEDDING model (e.g., MiniLM-L6-v3.gguf)
    bool loadEmbeddingModel(const std::string& modelPath);
    
    // Generate a normalized float vector (e.g., size 384)
    std::vector<float> getEmbedding(const std::string& text);
    
    // Helper: Get dimension of the model (e.g., 384)
    int getDimension() const;

private:
    LlamaEngine() = default;
    ~LlamaEngine();
    
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    const llama_vocab* m_vocab = nullptr;
};

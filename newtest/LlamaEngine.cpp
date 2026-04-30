#include "LlamaEngine.h"
#include <iostream>
#include <numeric>
#include <cmath>

LlamaEngine& LlamaEngine::instance() {
    static LlamaEngine engine;
    return engine;
}

LlamaEngine::~LlamaEngine() {
    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_model_free(m_model); // Corrected: llama_model_free
    llama_backend_free();
}

bool LlamaEngine::loadEmbeddingModel(const std::string& modelPath) {
    llama_backend_init();
    
    // Removed: model_params.embeddings assignment (it doesn't exist)
    llama_model_params model_params = llama_model_default_params();
    
    m_model = llama_model_load_from_file(modelPath.c_str(), model_params);
    if (!m_model) {
        std::cerr << "[Llama] Error: Failed to load model." << std::endl;
        return false;
    }
    m_vocab = llama_model_get_vocab(m_model);
    
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 512;
    ctx_params.n_batch = 512;
    ctx_params.n_ubatch = 512;
    ctx_params.embeddings = true;
    ctx_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    
    m_ctx = llama_init_from_model(m_model, ctx_params);
    if (!m_ctx) {
        std::cerr << "[Llama] Error: Failed to create context." << std::endl;
        return false;
    }
    
    std::cout << "[Llama] Embedding Model Loaded." << std::endl;
    return true;
}

int LlamaEngine::getDimension() const {
    return m_model ? llama_model_n_embd(m_model) : 0;
}

std::vector<float> LlamaEngine::getEmbedding(const std::string& text) {
    if (!m_ctx || !m_model || !m_vocab) return {};

    // 1. Tokenize Input
    // Added BOS (Begin of Sentence) because most embedding models expect it.
    std::vector<llama_token> tokens(text.length() + 1); 
    int n_tokens = llama_tokenize(m_vocab, text.c_str(), text.length(), tokens.data(), tokens.size(), true, true);
    
    // Fallback: If adding BOS fails (returns negative), try without BOS
    if (n_tokens < 0) {
        n_tokens = llama_tokenize(m_vocab, text.c_str(), text.length(), tokens.data(), tokens.size(), false, true);
    }

    if (n_tokens <= 0) {
        std::cerr << "[Llama] Tokenization failed for text: '" << text << "'" << std::endl;
        return {};
    }
    
    tokens.resize(n_tokens);

    // 2. Prepare Batch
    // Mark tokens as outputs so llama.cpp keeps the pooled sequence embedding.
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    batch.n_tokens = n_tokens;
    
    for (int i = 0; i < n_tokens; ++i) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = true;
    }

    // 3. Encode
    int result = llama_encode(m_ctx, batch);
    
    if (result != 0) {
        std::cerr << "[Llama] Encode failed with code: " << result << std::endl;
        llama_batch_free(batch);
        return {};
    }
    
    llama_batch_free(batch);

    // 4. Extract Embedding Vector
    // For embedding models, we usually take the output of the last token (seq_id 0)
    const float* embd_out = llama_get_embeddings_seq(m_ctx, 0);
    
    if (!embd_out) {
        std::cerr << "[Llama] Failed to retrieve embeddings." << std::endl;
        return {};
    }

    int dim = llama_model_n_embd(m_model);
    std::vector<float> result_vec(embd_out, embd_out + dim);
    
    // 5. Normalize Vector (L2 Normalization)
    float norm = std::sqrt(std::inner_product(result_vec.begin(), result_vec.end(), result_vec.begin(), 0.0f));
    if (norm > 0) {
        for (auto& v : result_vec) {
            v /= norm;
        }
    }
    
    return result_vec;
}

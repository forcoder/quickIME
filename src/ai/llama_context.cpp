#include "quickinput/ai/llama_context.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>
#include <windows.h>

// 条件编译：如果找到llama.cpp则使用真实实现，否则使用模拟实现
#if __has_include(<llama.h) || defined(HAS_LLAMACPP)
    #include <llama.h>
    #define USE_REAL_LLAMA 1
#else
    #define USE_REAL_LLAMA 0
#endif

namespace qi::ai {

// ─── 宽字符串转UTF-8 ───
static std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

static std::wstring UTF8ToWString(const std::string& str) {
    if (str.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

// ─── PIMPL Implementation ───
struct LlamaContext::Impl {
    bool isLoaded = false;
    std::wstring modelPath;
    LlamaParams currentParams;

#if USE_REAL_LLAMA
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const llama_vocab* vocab = nullptr;
#endif

    // 模拟模式下的数据
    size_t mockMemorySize = 0;

    Impl() = default;

    ~Impl() {
        UnloadModel();
    }

    void UnloadModel() {
#if USE_REAL_LLAMA
        if (ctx) {
            llama_free(ctx);
            ctx = nullptr;
        }
        if (model) {
            llama_model_free(model);
            model = nullptr;
        }
        vocab = nullptr;
#endif
        isLoaded = false;
        mockMemorySize = 0;
    }
};

LlamaContext::LlamaContext() : m_impl(std::make_unique<Impl>()) {}

LlamaContext::~LlamaContext() = default;

bool LlamaContext::LoadModel(const std::wstring& modelPath, const LlamaParams& params) {
    if (m_impl->isLoaded) {
        UnloadModel();
    }

    // 验证文件存在
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::wcerr << L"[LlamaContext] 无法打开模型文件: " << modelPath << std::endl;
        return false;
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.close();

    m_impl->modelPath = modelPath;
    m_impl->currentParams = params;
    m_impl->mockMemorySize = fileSize;

#if USE_REAL_LLAMA
    // ── 真实llama.cpp初始化 ──
    llama_model_params mparams = llama_model_default_params();
    mparams.n_gpu_layers = params.n_gpu_layers;

    std::string utf8Path = WStringToUTF8(modelPath);
    m_impl->model = llama_model_load_from_file(utf8Path.c_str(), mparams);
    if (!m_impl->model) {
        std::wcerr << L"[LlamaContext] llama.cpp 模型加载失败" << std::endl;
        return false;
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = params.nCtx;
    cparams.n_threads = params.nThreads;
    cparams.n_batch = params.batchSize;

    m_impl->ctx = llama_init_from_model(m_impl->model, cparams);
    if (!m_impl->ctx) {
        std::wcerr << L"[LlamaContext] llama.cpp 上下文创建失败" << std::endl;
        llama_model_free(m_impl->model);
        m_impl->model = nullptr;
        return false;
    }

    m_impl->vocab = llama_model_get_vocab(m_impl->model);
    m_impl->isLoaded = true;

    std::wcout << L"[LlamaContext] llama.cpp 模型加载成功: " << modelPath << std::endl;
#else
    // ── 模拟模式 ──
    m_impl->isLoaded = true;
    std::wcout << L"[LlamaContext] 模拟模式 - 模型加载成功: " << modelPath << std::endl;
#endif

    return true;
}

void LlamaContext::UnloadModel() {
    m_impl->UnloadModel();
}

bool LlamaContext::IsModelLoaded() const {
    return m_impl->isLoaded;
}

LlamaResult LlamaContext::Generate(const std::string& prompt) {
    LlamaResult result;

    if (!m_impl->isLoaded) {
        result.error = "模型未加载";
        return result;
    }

    auto startTime = std::chrono::steady_clock::now();

#if USE_REAL_LLAMA
    // ── 真实llama.cpp推理 ──
    std::vector<llama_token> inputTokens;
    const int nPrompt = -1; // 自动计算

    // Tokenize输入
    inputTokens.resize(prompt.size() + 16);
    int nTokens = llama_tokenize(m_impl->vocab, prompt.c_str(), prompt.size(),
                                  inputTokens.data(), inputTokens.size(), true, true);
    if (nTokens < 0) {
        result.error = "Tokenization失败";
        return result;
    }
    inputTokens.resize(nTokens);

    // 创建batch
    llama_batch batch = llama_batch_init(m_impl->currentParams.batchSize, 0, 1);
    for (int i = 0; i < nTokens; ++i) {
        batch.token[i] = inputTokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == nTokens - 1); // 只在最后一个token计算logits
    }
    batch.n_tokens = nTokens;

    // 前向传播
    if (llama_decode(m_impl->ctx, batch) != 0) {
        result.error = "llama_decode失败";
        llama_batch_free(batch);
        return result;
    }

    // 生成循环
    std::string output;
    int generated = 0;
    llama_token newTokenId = 0;

    // 采样器参数
    llama_sampler_chain_params sparams = llama_sampler_chain_default_params();
    sparams.no_perf = false;
    auto* sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(m_impl->currentParams.topK));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(m_impl->currentParams.topP, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(m_impl->currentParams.temperature));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(0));

    while (generated < m_impl->currentParams.maxTokens) {
        newTokenId = llama_sampler_sample(sampler, m_impl->ctx, -1);

        // 检查EOS
        if (llama_vocab_is_eog(m_impl->vocab, newTokenId)) {
            break;
        }

        // Token转文本
        char buf[256];
        int len = llama_token_to_piece(m_impl->vocab, newTokenId, buf, sizeof(buf), 0, true);
        if (len > 0) {
            output.append(buf, len);
        }

        // 准备下一个batch
        batch.n_tokens = 1;
        batch.token[0] = newTokenId;
        batch.pos[0] = nTokens + generated;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0] = true;

        if (llama_decode(m_impl->ctx, batch) != 0) {
            break;
        }

        ++generated;

        // 流式回调
        if (m_impl->currentParams.onToken && len > 0) {
            m_impl->currentParams.onToken(std::string(buf, len));
        }
    }

    llama_sampler_free(sampler);
    llama_batch_free(batch);

    result.success = true;
    result.output = output;
    result.tokensGenerated = generated;

#else
    // ── 模拟推理 ──
    // 模拟延迟
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 根据prompt内容生成模拟回复
    std::string mockOutput;
    if (prompt.find("客服") != std::string::npos || prompt.find("customer") != std::string::npos) {
        mockOutput = "您好，感谢您的咨询，我们会尽快为您处理。";
    } else if (prompt.find("FAQ") != std::string::npos || prompt.find("常见问题") != std::string::npos) {
        mockOutput = "请查看我们的帮助中心获取详细解答。";
    } else if (prompt.find("订单") != std::string::npos || prompt.find("order") != std::string::npos) {
        mockOutput = "您的订单正在处理中，预计3-5个工作日送达。";
    } else if (prompt.find("投诉") != std::string::npos || prompt.find("complaint") != std::string::npos) {
        mockOutput = "非常抱歉给您带来不便，我们会立即核实并处理。";
    } else {
        mockOutput = "好的，我已收到您的信息，正在为您处理。";
    }

    result.success = true;
    result.output = mockOutput;
    result.tokensGenerated = 10; // 模拟值
#endif

    auto endTime = std::chrono::steady_clock::now();
    result.inferenceTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    return result;
}

bool LlamaContext::GenerateStreaming(const std::string& prompt, const LlamaParams& params) {
    if (!m_impl->isLoaded) {
        return false;
    }

    // 流式推理：设置回调后调用Generate
    LlamaParams streamParams = params;
    // 注意：Generate内部会调用onToken回调
    auto result = Generate(prompt);
    return result.success;
}

std::optional<LlamaContext::ModelInfo> LlamaContext::GetModelInfo() const {
    if (!m_impl->isLoaded) {
        return std::nullopt;
    }

    ModelInfo info;
    info.path = m_impl->modelPath;
    info.memorySize = m_impl->mockMemorySize;
    info.nCtx = m_impl->currentParams.nCtx;
    info.nTokens = 0;

#if USE_REAL_LLAMA
    if (m_impl->vocab) {
        info.nTokens = llama_vocab_n_tokens(m_impl->vocab);
    }
    if (m_impl->model) {
        info.modelArch = llama_model_desc(m_impl->model, nullptr, 0);
    }
#else
    info.nTokens = 32000; // 模拟值
    info.modelArch = "mock-llama";
#endif

    return info;
}

} // namespace qi::ai

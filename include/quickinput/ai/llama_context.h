#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <optional>

#if defined(_WIN32)
    #define LLAMA_SHARED_LIBRARY
    #ifdef LLAMA_BUILDING_LIBRARY
        #define LLAMA_API __declspec(dllexport)
    #else
        #define LLAMA_API __declspec(dllimport)
    #endif
#else
    #define LLAMA_API
#endif

namespace qi::ai {

// Llama.cpp 推理参数
struct LlamaParams {
    int nCtx = 2048;           // 上下文窗口大小
    int nThreads = 4;           // CPU 线程数
    int n_gpu_layers = 0;       // GPU 加速层数 (0=仅CPU)
    float temperature = 0.7f;  // 温度参数
    int maxTokens = 128;       // 最大生成长度
    float repeatPenalty = 1.1f; // 重复惩罚
    int batchSize = 512;        // 批处理大小
    float topP = 0.95f;         // Top-p 采样
    int topK = 40;              // Top-k 采样
    float frequencyPenalty = 0.0f;
    float presencePenalty = 0.0f;

    // 流式输出回调 (每生成一个token调用一次)
    std::function<void(const std::string& token)> onToken;

    LlamaParams() = default;
};

// Llama.cpp 推理结果
struct LlamaResult {
    bool success;
    std::string output;
    std::string error;
    int tokensGenerated;
    float inferenceTimeMs;

    LlamaResult() : success(false), tokensGenerated(0), inferenceTimeMs(0.0f) {}
};

// Llama模型上下文
class LlamaContext {
public:
    LlamaContext();
    ~LlamaContext();

    // 加载模型
    bool LoadModel(const std::wstring& modelPath, const LlamaParams& params);

    // 卸载模型
    void UnloadModel();

    // 检查模型是否已加载
    bool IsModelLoaded() const;

    // 推理（完整输出）
    LlamaResult Generate(const std::string& prompt);

    // 流式推理
    bool GenerateStreaming(const std::string& prompt, const LlamaParams& params);

    // 获取模型信息
    struct ModelInfo {
        std::wstring path;
        size_t memorySize;
        int nCtx;
        int nTokens;
        std::string modelArch;
    };
    std::optional<ModelInfo> GetModelInfo() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace qi::ai

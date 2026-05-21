#pragma once

#include <string>
#include <memory>

namespace qi::ai {

// 模型类型枚举
enum class ModelType {
    LlamaCpp,       // llama.cpp .gguf 格式
    ONNXRuntime,    // ONNX Runtime .onnx/.bin 格式
    Unknown
};

// 模型信息结构
struct ModelInfo {
    std::wstring path;              // 模型路径
    ModelType type;                 // 模型类型
    size_t memorySize;             // 模型大小（字节）
    int64_t version;                // 版本号
    bool isQuantized;               // 是否量化
    std::string quantization;       // 量化类型 (Q4_K_M 等)

    ModelInfo() : memorySize(0), version(0), isQuantized(false) {}
};

// 模型加载结果
struct LoadResult {
    bool success;
    std::wstring error;
    ModelInfo modelInfo;

    LoadResult() : success(false) {}
};

class ModelLoader {
public:
    ModelLoader();
    ~ModelLoader();

    // 检测模型类型
    static ModelType DetectModelType(const std::wstring& modelPath);

    // 获取模型信息
    LoadResult GetModelInfo(const std::wstring& modelPath);

    // 检查文件是否存在且可读
    static bool ValidateModelFile(const std::wstring& modelPath);

    // 获取支持的扩展名
    static std::vector<std::wstring> GetSupportedExtensions();

    // 格式化模型大小显示
    static std::wstring FormatMemorySize(size_t bytes);

private:
    // 根据文件扩展名判断模型类型
    static ModelType InferModelTypeFromExtension(const std::wstring& filePath);

    // 验证llama.cpp模型
    static LoadResult ValidateLlamaCppModel(const std::wstring& modelPath);

    // 验证ONNX模型
    static LoadResult ValidateONNXModel(const std::wstring& modelPath);
};

} // namespace qi::ai
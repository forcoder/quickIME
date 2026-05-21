#include "quickinput/ai/model_loader.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace qi::ai {

ModelLoader::ModelLoader() {}

ModelLoader::~ModelLoader() {}

ModelType ModelLoader::DetectModelType(const std::wstring& modelPath) {
    if (modelPath.empty()) {
        return ModelType::Unknown;
    }

    // 获取文件扩展名
    size_t dotPos = modelPath.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        return ModelType::Unknown;
    }

    std::wstring extension = modelPath.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](wchar_t c) { return std::towlower(c); });

    // 根据扩展名判断模型类型
    if (extension == L"gguf" || extension == L"bin") {
        return ModelType::LlamaCpp;
    } else if (extension == L"onnx") {
        return ModelType::ONNXRuntime;
    }

    return ModelType::Unknown;
}

LoadResult ModelLoader::GetModelInfo(const std::wstring& modelPath) {
    LoadResult result;

    // 验证文件存在性
    if (!ValidateModelFile(modelPath)) {
        result.error = L"模型文件不存在或不可读: " + modelPath;
        return result;
    }

    ModelType type = DetectModelType(modelPath);

    switch (type) {
        case ModelType::LlamaCpp:
            result = ValidateLlamaCppModel(modelPath);
            break;
        case ModelType::ONNXRuntime:
            result = ValidateONNXModel(modelPath);
            break;
        default:
            result.success = false;
            result.error = L"不支持的模型格式";
            break;
    }

    if (result.success) {
        result.modelInfo.path = modelPath;
        result.modelInfo.type = type;
    }

    return result;
}

bool ModelLoader::ValidateModelFile(const std::wstring& modelPath) {
    if (modelPath.empty()) {
        return false;
    }

    // 检查文件是否存在
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    // 检查文件大小（至少1KB）
    std::streamsize size = file.tellg();
    file.close();

    return size > 1024;
}

std::vector<std::wstring> ModelLoader::GetSupportedExtensions() {
    return {L"gguf", L"bin", L"onnx"};
}

std::wstring ModelLoader::FormatMemorySize(size_t bytes) {
    const size_t KB = 1024;
    const size_t MB = KB * 1024;
    const size_t GB = MB * 1024;

    if (bytes >= GB) {
        double gb = static_cast<double>(bytes) / GB;
        wchar_t buffer[32];
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.2f GB", gb);
        return std::wstring(buffer);
    } else if (bytes >= MB) {
        double mb = static_cast<double>(bytes) / MB;
        wchar_t buffer[32];
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.2f MB", mb);
        return std::wstring(buffer);
    } else if (bytes >= KB) {
        double kb = static_cast<double>(bytes) / KB;
        wchar_t buffer[32];
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%.2f KB", kb);
        return std::wstring(buffer);
    } else {
        wchar_t buffer[32];
        swprintf(buffer, sizeof(buffer)/sizeof(wchar_t), L"%zu B", bytes);
        return std::wstring(buffer);
    }
}

ModelType ModelLoader::InferModelTypeFromExtension(const std::wstring& filePath) {
    return DetectModelType(filePath);
}

LoadResult ModelLoader::ValidateLlamaCppModel(const std::wstring& modelPath) {
    LoadResult result;
    result.success = true;

    // 模拟读取llama.cpp模型信息
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        result.success = false;
        result.error = L"无法打开llama.cpp模型文件";
        return result;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 模拟读取头部信息
    char header[64] = {0};
    file.read(header, sizeof(header));

    result.modelInfo.memorySize = static_cast<size_t>(size);
    result.modelInfo.version = 100; // 模拟版本号
    result.modelInfo.isQuantized = (strstr(header, "Q4_K_M") != nullptr);
    result.modelInfo.quantization = result.modelInfo.isQuantized ? L"Q4_K_M" : L"FP16";

    file.close();
    return result;
}

LoadResult ModelLoader::ValidateONNXModel(const std::wstring& modelPath) {
    LoadResult result;
    result.success = true;

    // 模拟读取ONNX模型信息
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        result.success = false;
        result.error = L"无法打开ONNX模型文件";
        return result;
    }

    std::streamsize size = file.tellg();
    result.modelInfo.memorySize = static_cast<size_t>(size);
    result.modelInfo.version = 100;
    result.modelInfo.isQuantized = false;
    result.modelInfo.quantization = L"FP32";

    file.close();
    return result;
}

} // namespace qi::ai
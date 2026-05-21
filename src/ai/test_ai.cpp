#include "quickinput/ai/inference_engine.h"
#include "quickinput/ai/prompt_templates.h"
#include "quickinput/ai/model_loader.h"
#include <iostream>
#include <chrono>

int main() {
    std::wcout << L"=== QuickInput AI 推理接口测试 ===" << std::endl;
    std::wcout << std::endl;

    // 测试模型加载器
    std::wcout << L"1. 测试模型加载器..." << std::endl;
    qi::ai::ModelLoader loader;

    // 检测支持的扩展名
    auto extensions = loader.GetSupportedExtensions();
    std::wcout << L"支持的模型格式: ";
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (i > 0) std::wcout << L", ";
        std::wcout << extensions[i];
    }
    std::wcout << std::endl;

    // 模拟模型文件信息
    std::vector<std::pair<std::wstring, qi::ai::ModelType>> mockModels = {
        {L"C:\\models\\llama.gguf", qi::ai::ModelType::LlamaCpp},
        {L"C:\\models\\onnx_model.onnx", qi::ai::ModelType::ONNXRuntime}
    };

    for (const auto& model : mockModels) {
        qi::ai::LoadResult result = loader.GetModelInfo(model.first);
        if (result.success) {
            std::wcout << L"模型类型检测: " << model.first << L" -> ";
            switch (model.second) {
                case qi::ai::ModelType::LlamaCpp:
                    std::wcout << L"Llama.cpp";
                    break;
                case qi::ai::ModelType::ONNXRuntime:
                    std::wcout << L"ONNX Runtime";
                    break;
                default:
                    std::wcout << L"未知";
                    break;
            }
            std::wcout << std::endl;
            std::wcout << L"模型大小: " << loader.FormatMemorySize(result.modelInfo.memorySize) << std::endl;
        } else {
            std::wcerr << L"模型验证失败: " << result.error << std::endl;
        }
    }

    std::wcout << std::endl;

    // 测试提示词模板管理器
    std::wcout << L"2. 测试提示词模板管理器..." << std::endl;
    auto supportedTypes = qi::ai::PromptTemplateManager::GetSupportedTypes();
    for (auto type : supportedTypes) {
        auto template_ = qi::ai::PromptTemplateManager::GetTemplate(type);
        std::wcout << L"类型: ";
        switch (type) {
            case qi::ai::AISuggestionType::ShortReply:
                std::wcout << L"短句回复";
                break;
            case qi::ai::AISuggestionType::WorkPhrase:
                std::wcout << L"工作话术";
                break;
            case qi::ai::AISuggestionType::DailyExpression:
                std::wcout << L"日常用语";
                break;
            case qi::ai::AISuggestionType::ProfessionalText:
                std::wcout << L"专业文案";
                break;
            default:
                std::wcout << L"未知";
                break;
        }
        std::wcout << L" - " << qi::ai::PromptTemplateManager::GetTypeDescription(type) << std::endl;
        std::wcout << L"系统提示: " << template_.systemPrompt.substr(0, 50) << L"..." << std::endl;
    }

    std::wcout << std::endl;

    // 测试推理引擎
    std::wcout << L"3. 测试推理引擎..." << std::endl;
    qi::ai::InferenceEngine engine;

    // 模拟初始化（使用不存在的模型路径，仅测试接口）
    std::wstring mockModelPath = L"C:\\models\\mock_model.gguf";
    bool initialized = engine.Initialize(mockModelPath);
    std::wcout << L"初始化结果: " << (initialized ? L"成功" : L"失败") << std::endl;

    if (initialized && engine.IsModelLoaded()) {
        std::wcout << L"模型加载状态: 已加载" << std::endl;

        // 生成AI建议
        std::wstring context = L"用户正在与客户沟通项目进度";
        std::wstring userInput = L"请帮我生成一些合适的回复";

        auto suggestions = engine.GenerateSuggestions(context, userInput, 3);

        std::wcout << L"\n生成的AI建议:" << std::endl;
        for (size_t i = 0; i < suggestions.size(); ++i) {
            const auto& suggestion = suggestions[i];

            std::wcout << L"建议 " << (i + 1) << L": ";
            switch (suggestion.type) {
                case qi::ai::AISuggestionType::ShortReply:
                    std::wcout << L"[短句回复] ";
                    break;
                case qi::ai::AISuggestionType::WorkPhrase:
                    std::wcout << L"[工作话术] ";
                    break;
                case qi::ai::AISuggestionType::DailyExpression:
                    std::wcout << L"[日常用语] ";
                    break;
                case qi::ai::AISuggestionType::ProfessionalText:
                    std::wcout << L"[专业文案] ";
                    break;
                default:
                    break;
            }

            std::wcout << L"文本: " << suggestion.text
                      << L" | 置信度: " << static_cast<int>(suggestion.confidence * 100) << L"%"
                      << std::endl;
        }

        // 测试不同类型生成
        std::wcout << L"\n测试不同类型生成:" << std::endl;

        auto shortReplies = engine.GenerateShortReplies(L"聊天对话", 2);
        std::wcout << L"短句回复 (" << shortReplies.size() << L"): ";
        for (const auto& reply : shortReplies) {
            std::wcout << reply.text << L" ";
        }
        std::wcout << std::endl;

        auto workPhrases = engine.GenerateWorkPhrases(L"工作会议", 2);
        std::wcout << L"工作话术 (" << workPhrases.size() << L"): ";
        for (const auto& phrase : workPhrases) {
            std::wcout << phrase.text << L" ";
        }
        std::wcout << std::endl;

        auto dailyExpressions = engine.GenerateDailyExpressions(L"日常生活", 2);
        std::wcout << L"日常用语 (" << dailyExpressions.size() << L"): ";
        for (const auto& expr : dailyExpressions) {
            std::wcout << expr.text << L" ";
        }
        std::wcout << std::endl;
    }

    std::wcout << std::endl;

    // 性能测试
    std::wcout << L"4. 性能测试..." << std::endl;
    if (engine.IsModelLoaded()) {
        auto start = std::chrono::high_resolution_clock::now();

        // 生成10次建议
        for (int i = 0; i < 10; ++i) {
            engine.GenerateSuggestions(L"测试上下文", L"测试输入", 3);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::wcout << L"生成10次建议耗时: " << duration.count() << L" ms" << std::endl;
        std::wcout << L"平均每次: " << (duration.count() / 10) << L" ms" << std::endl;
    }

    std::wcout << std::endl;

    // 清理
    engine.Shutdown();
    std::wcout << L"测试完成" << std::endl;

    return 0;
}
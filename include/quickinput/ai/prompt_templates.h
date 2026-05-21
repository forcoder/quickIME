#pragma once

#include <string>
#include <vector>
#include "inference_engine.h"

namespace qi::ai {

// 提示词模板管理器
class PromptTemplateManager {
public:
    // 获取所有支持的模板类型
    static std::vector<AISuggestionType> GetSupportedTypes();

    // 获取指定类型的模板
    static PromptTemplate GetTemplate(AISuggestionType type);

    // 验证模板配置是否有效
    static bool ValidateTemplate(const PromptTemplate& template);

    // 获取模板描述信息
    static std::wstring GetTypeDescription(AISuggestionType type);

private:
    // 预定义的模板数据
    static const std::vector<PromptTemplate> s_templates;
};

} // namespace qi::ai
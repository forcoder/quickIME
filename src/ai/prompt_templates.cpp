#include "quickinput/ai/prompt_templates.h"
#include <algorithm>
#include <cctype>

namespace qi::ai {

const std::vector<PromptTemplate> PromptTemplateManager::s_templates = {
    // 短句回复模板
    {
        L"你是一个专业的五笔输入法智能辅助助手，专门帮助用户生成简短、实用的回复。请根据上下文和用户输入，生成2-15字的简短回复。要求回复简洁明了，符合中文表达习惯，避免冗长和重复。",
        L"基于以上上下文和用户输入，请生成一个合适的简短回复：",
        L"请直接输出回复内容，不需要额外的标点符号或解释。",
        50, // maxTokens
        0.7f // temperature
    },

    // 工作话术模板
    {
        L"你是一名专业的职场沟通助手，擅长处理各种工作场景下的沟通需求。请根据工作场景，生成专业、得体的工作话术。要求语言正式、礼貌，适合商务场合使用。",
        L"当前工作场景：",
        L"请生成合适的工作话术，保持专业性和礼貌性。",
        80,
        0.6f
    },

    // 日常用语模板
    {
        L"你是一个贴心的日常聊天助手，能够生成温馨、自然的日常对话。请根据生活场景，生成亲切、友好的日常用语。要求语言口语化，富有生活气息，让人感到温暖和关怀。",
        L"日常生活场景：",
        L"请生成温馨的日常对话内容。",
        60,
        0.8f
    },

    // 专业文案模板
    {
        L"你是一位资深的内容创作专家，具备深厚的行业知识和文案撰写能力。请根据主题和要求，生成高质量的专业文案。要求内容准确、逻辑清晰，具有专业水准。",
        L"专业文案主题：",
        L"请生成专业的文案内容，确保信息准确、表达专业。",
        100,
        0.5f
    },

    // 客服回复模板
    {
        L"你是一位专业的在线客服代表，负责处理客户咨询和回复。请根据客户消息和对话上下文，生成简洁、礼貌、专业的客服回复。要求：1) 语气亲切友好 2) 回答准确专业 3) 每条回复不超过30字 4) 提供3条不同角度的回复建议",
        L"对话上下文：\n{context}\n\n客户消息：\n{customer_msg}",
        L"请生成3条客服回复建议，每条回复简洁实用，适合客服场景。",
        150,
        0.7f
    },

    // FAQ 回复模板
    {
        L"你是客服知识库助手，根据用户问题和知识库匹配内容，生成准确的FAQ回复。要求回答简洁明了，直接解决用户疑问。",
        L"用户问题：{question}\n\n知识库匹配：{kb_match}",
        L"请生成基于知识库的精准回复，不超过40字。",
        100,
        0.3f
    },

    // 投诉处理模板
    {
        L"你是一位经验丰富的客服主管，专门处理客户投诉。请根据投诉内容和上下文，生成妥善的投诉处理回复。要求：1) 表达歉意和理解 2) 提供解决方案 3) 语气真诚专业 4) 每条不超过35字",
        L"投诉内容：\n{complaint}\n\n对话上下文：\n{context}",
        L"请生成3条投诉处理回复，既要安抚客户情绪，又要提供解决方案。",
        150,
        0.6f
    },

    // 订单咨询模板
    {
        L"你是订单客服专员，负责解答客户关于订单状态、发货、物流等问题。请根据客户咨询生成专业回复。",
        L"客户咨询：{inquiry}",
        L"请生成准确、专业的订单咨询回复，包含必要的订单信息指引。",
        120,
        0.5f
    },

    // 退款流程模板
    {
        L"你是退款客服专员，负责处理退款申请。请根据退款请求生成流程说明和安抚回复。要求：1) 说明退款流程和时间 2) 表达理解 3) 提供退款进度查询方式",
        L"退款请求：{refund_request}",
        L"请生成退款流程说明和安抚回复，告知预计到账时间和查询方式。",
        120,
        0.5f
    }
};

std::vector<AISuggestionType> PromptTemplateManager::GetSupportedTypes() {
    return {
        AISuggestionType::ShortReply,
        AISuggestionType::WorkPhrase,
        AISuggestionType::DailyExpression,
        AISuggestionType::ProfessionalText,
        AISuggestionType::CustomerService,
        AISuggestionType::FaqReply,
        AISuggestionType::ComplaintHandle,
        AISuggestionType::OrderInquiry,
        AISuggestionType::RefundProcess
    };
}

PromptTemplate PromptTemplateManager::GetTemplate(AISuggestionType type) {
    size_t index = static_cast<size_t>(type);
    if (index < s_templates.size()) {
        return s_templates[index];
    }

    // 默认模板
    return PromptTemplate{
        L"你是一个AI助手",
        L"请输入内容：",
        L"请直接输出结果",
        50,
        0.7f
    };
}

bool PromptTemplateManager::ValidateTemplate(const PromptTemplate& template_) {
    // 验证系统提示不为空
    if (template_.systemPrompt.empty()) {
        return false;
    }

    // 验证最大token数为正数
    if (template_.maxTokens <= 0) {
        return false;
    }

    // 验证温度参数在合理范围内
    if (template_.temperature < 0.0f || template_.temperature > 1.0f) {
        return false;
    }

    return true;
}

std::wstring PromptTemplateManager::GetTypeDescription(AISuggestionType type) {
    switch (type) {
        case AISuggestionType::ShortReply:
            return L"短句回复 - 2-15字的简短实用回复";
        case AISuggestionType::WorkPhrase:
            return L"工作话术 - 专业的职场沟通用语";
        case AISuggestionType::DailyExpression:
            return L"日常用语 - 温馨自然的日常对话";
        case AISuggestionType::ProfessionalText:
            return L"专业文案 - 高质量的专业内容";
        case AISuggestionType::CustomerService:
            return L"客服回复 - 专业的在线客服回复";
        case AISuggestionType::FaqReply:
            return L"FAQ回复 - 基于知识库的精准回复";
        case AISuggestionType::ComplaintHandle:
            return L"投诉处理 - 妥善的投诉处理回复";
        case AISuggestionType::OrderInquiry:
            return L"订单咨询 - 订单状态和物流咨询";
        case AISuggestionType::RefundProcess:
            return L"退款流程 - 退款申请处理";
        default:
            return L"未知类型";
    }
}

} // namespace qi::ai
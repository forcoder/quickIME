#include "quickinput/knowledge/knowledge_base.h"
#include <iostream>

int main() {
    qi::knowledge::KnowledgeBase kb;
    std::wstring dbPath = L"C:\\QuickInput\\knowledge\\test.db";

    if (kb.Initialize(dbPath)) {
        std::cout << "知识库初始化成功" << std::endl;

        // 导入文本
        kb.ImportText(L"这是测试文本。包含一些中文内容。", qi::knowledge::KbCategory::General);

        // 搜索
        auto results = kb.Search(L"测试", 5);
        std::wcout << L"搜索结果: " << results.size() << L" 条" << std::endl;

        // 获取统计
        auto stats = kb.GetStats();
        for (const auto& s : stats) {
            std::wcout << L"类别 " << static_cast<int>(s.category)
                      << L": " << s.entryCount << L" 条目" << std::endl;
        }

        kb.Shutdown();
    } else {
        std::cerr << "知识库初始化失败" << std::endl;
    }

    return 0;
}
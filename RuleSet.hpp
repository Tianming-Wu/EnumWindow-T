#pragma once


#include <fstream>
#include <vector>
#include <string>
#include <json/json.h>
#include <optional>

#include <regex>


enum class PatternKey : uint8_t {
    Class, Title
};

struct FilterRule {
    std::regex pattern;
    PatternKey key;
    std::optional<std::string> description; // 可选的规则描述

    bool match(const std::string& target) const;
};

class _Filter {
public:
    _Filter();

    // 设置文件名，返回当前文件是否存在。
    bool setFileName(const std::string& filename);

    void load();
    void save();

    // 创建规则集的备份文件
    void backupRuleSet();

    // 重置规则集（到默认状态）
    void resetRuleSet();

    // 增量插入：不重构整个规则集
    void insertRule(const std::string& pattern, PatternKey key, std::optional<std::string> description = std::nullopt);

    void insertTempRule(const std::string& pattern, PatternKey key);

    bool match(const std::string& windowClass, const std::string& windowTitle);

protected:

    void parseRuleSet();

private:
    std::vector<FilterRule> ruleSet;
    std::vector<FilterRule> tempRuleSet; // 临时规则集，仅在本次启动生效。后续可能需要处理提权时的传递问题。

    std::string m_filename;
    Json::Value root;

};


extern _Filter RuleSet;
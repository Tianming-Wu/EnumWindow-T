#include "RuleSet.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "toolset.h"

#include <windows.h>


// Global reference
_Filter RuleSet;

_Filter::_Filter()
    : m_filename((getExecutableDir()/"filter.json").string())
{
}

bool _Filter::setFileName(const std::string &filename)
{
    m_filename = filename;
    return fs::exists(m_filename);
}

void _Filter::load()
{
    if (!fs::exists(m_filename)) {
        // 文件不存在，静默重置
        resetRuleSet();
        return;
    }
    std::ifstream file(m_filename);
    if (file.bad()) {
        // 文件无法加载
        MessageBoxA(NULL, "无法加载规则集", "警告", MB_ICONWARNING);
        resetRuleSet();
    }
    file.open(m_filename);
    file >> root;
    file.close();

    parseRuleSet();
}

void _Filter::save()
{
    // 使用 jsoncpp 的 StreamWriterBuilder 直接输出 UTF-8 文本，避免将中文转为 \uXXXX 转义
    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true;
    builder["indentation"] = "    ";
    std::string output = Json::writeString(builder, root);

    std::ofstream file(m_filename, std::ios::binary);
    if(!file.is_open()) return;
    file.write(output.c_str(), static_cast<std::streamsize>(output.size()));
    file.close();
}

bool _Filter::match(const std::string &windowClass, const std::string &windowTitle)
{
    // 检查规则集，如果含有匹配则返回 true。
    // 由于这个规则集并没有动作的概念（只要匹配就是确认过滤），所以顺序并不重要，
    // 只会在大量规则时对性能有影响。性能影响很大程度上可以通过优化 Regex 规避。

    // 检查临时规则集
    for (const FilterRule& rule : tempRuleSet) {
        const std::string& targetString = (rule.key == PatternKey::Class) ? (windowClass) : (windowTitle);
        if (rule.match(targetString)) return true; 
    }

    // 检查规则集
    for (const FilterRule& rule : ruleSet) {
        const std::string& targetString = (rule.key == PatternKey::Class) ? (windowClass) : (windowTitle);
        if (rule.match(targetString)) return true; 
    }

    // 没有匹配项目
    return false;
}

void _Filter::parseRuleSet()
{
    int errorCount = 0;
    ruleSet.clear();

    if (root.isMember("ruleset") && root["ruleset"].isArray()) {
        for(const auto& rule : root["ruleset"]) {
            // 规则单元格式：
            // { "pattern": "<regex_expression>", "key": "c lass|title"}.

            if(rule.isMember("pattern") && rule.isMember("key") && rule["pattern"].isString() && rule["key"].isString()) {
                FilterRule fr;
                fr.pattern = rule["pattern"].asString();
                
                const std::string& key = rule["key"].asString();
                if (key == "class") {
                    fr.key = PatternKey::Class;
                } else if (key == "title") {
                    fr.key = PatternKey::Title;
                } else {
                    // 无效的规则单元，忽略。
                    errorCount++;
                    continue;
                }

                if (rule.isMember("description") && rule["description"].isString()) {
                    fr.description = rule["description"].asString();
                }

                ruleSet.push_back(fr);

            } else {
                // 无效的规则单元，忽略。
                errorCount++;
            }
        }
    } else {
        // 规则集不存在或者无效，恢复默认规则集。
        // 在那之前，先备份当前规则集。
        MessageBoxA(NULL, "待加载的规则集无效。", "警告", MB_ICONWARNING);
        backupRuleSet();
        resetRuleSet();
    }

    // 警告用户
    if (errorCount > 0) {
        MessageBoxA(NULL, std::string("在加载过滤规则集时遇到了 " + std::to_string(errorCount) + " 个错误。").c_str(), "警告", MB_ICONWARNING);
    }
}

void _Filter::backupRuleSet()
{
    fs::copy(m_filename, m_filename + ".bak");
}

void _Filter::resetRuleSet()
{
    // 将规则集重置到默认状态

    ruleSet.clear();
    root.clear();

    insertRule("^tooltips?_class32$", PatternKey::Class, "工具提示");
    insertRule("^(MSCTF)?IME$", PatternKey::Class, "输入法");
    insertRule("^ThumbnailDeviceHelperWnd$", PatternKey::Class, "缩略图设备帮助程序");
    insertRule("^ForegroundStaging$", PatternKey::Class, "前景暂存");
    // insertRule("^$", PatternKey::Class);
    // insertRule("^$", PatternKey::Class);
}

void _Filter::insertRule(const std::string &pattern, PatternKey key, std::optional<std::string> description)
{
    FilterRule newRule;

    newRule.pattern = pattern;
    newRule.key = key;
    newRule.description = description;

    // 向 ruleSet 中插入新建的规则
    ruleSet.push_back(newRule);

    // 向 root 中插入新建的规则 (增量)
    Json::Value newRuleVal;
    newRuleVal["pattern"] = pattern;
    newRuleVal["key"] = (key == PatternKey::Class) ? "class" : "title";

    if (description) {
        newRuleVal["description"] = *description;
    }

    root["ruleset"].append(newRuleVal);
}

void _Filter::insertTempRule(const std::string &pattern, PatternKey key)
{
    FilterRule newRule;

    newRule.pattern = pattern;
    newRule.key = key;

    // 向 tempRuleSet 中插入新建的临时规则
    tempRuleSet.push_back(newRule);
}

bool FilterRule::match(const std::string &target) const
{
    return std::regex_search(target, pattern);
}

#include "blocklist_fx.h"
#include <filesystem>

_BlockList BlockList;

_BlockList::_BlockList() {}

void _BlockList::load() {
    if(!std::filesystem::exists(filename)) write_default();
    std::ifstream file(filename);
    if(file.bad()) write_default();
    file.open(filename);
    file >> root;
    file.close();
    parseBlockList();
}

void _BlockList::setFileName(std::string& _filename) {
    filename = _filename;
}

bool _BlockList::exists(const std::string& item, const Json::Value::Members& list) {
    return std::find(list.begin(), list.end(), item) != list.end();
}

bool _BlockList::coexists(const std::string& item, const Json::Value::Members& list) {
    auto it = std::find_if(list.begin(), list.end(), [&item](const std::string& listItem) {
        return listItem.find(item) != std::string::npos;
    });
    return it != list.end();
}

void _BlockList::add(const std::string& item, std::vector<std::string>& list) {
    if (!exists(item, list)) {
        list.push_back(item);
    }
}

void _BlockList::remove(const std::string& item, Json::Value::Members& list) {
    auto it = std::find(list.begin(), list.end(), item);
    if (it != list.end()) {
        list.erase(it);
    }
}

void _BlockList::save() {
    updateBlockList();
    std::ofstream file(filename);
    file << root;
    file.close();
}

void _BlockList::parseBlockList() {
    if (root.isMember("blocklist")) {
        const Json::Value& blocklist = root["blocklist"];
        if (blocklist.isMember("WindowClass")) {
            const Json::Value& windowClassList = blocklist["WindowClass"];
            for (const Json::Value& item : windowClassList) {
                BLWindowClass.push_back(item.asString());
            }
        }
        if (blocklist.isMember("WindowTitle")) {
            const Json::Value& windowTitleList = blocklist["WindowTitle"];
            for (const Json::Value& item : windowTitleList) {
                BLWindowTitle.push_back(item.asString());
            }
        }
    }
}

void _BlockList::updateBlockList() {
    root["blocklist"]["WindowClass"].clear();
    root["blocklist"]["WindowTitle"].clear();
    for (const std::string& item : BLWindowClass) {
        root["blocklist"]["WindowClass"].append(item);
    }
    for (const std::string& item : BLWindowTitle) {
        root["blocklist"]["WindowTitle"].append(item);
    }
}

void _BlockList::write_default() {
    save();
}

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <json/json.h>

class _BlockList {
private:
    std::string filename = "./blocklist.json";
    Json::Value root;

public:
    std::vector<std::string> BLWindowClass;
    std::vector<std::string> BLWindowTitle;

    std::vector<std::string> temp_BLWindowClass;
    std::vector<std::string> temp_BLWindowTitle;

    _BlockList();

    void setFileName(std::string& _filename);

    void load();

    bool exists(const std::string& item, const Json::Value::Members& list);
    bool coexists(const std::string& item, const Json::Value::Members& list);
    void add(const std::string& item, std::vector<std::string>& list);
    void remove(const std::string& item, Json::Value::Members& list);
    void save();

    inline bool hasClass(const std::string& item) { return exists(item, BLWindowClass); }
    inline bool hasTitle(const std::string& item) { return exists(item, BLWindowTitle); }

private:
    void parseBlockList();
    void updateBlockList();
};

extern _BlockList BlockList;
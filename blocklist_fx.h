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
    void save();

    void addClass(const std::string &name) { add(name, temp_BLWindowClass); };
    void addTitle(const std::string &name) { add(name, temp_BLWindowTitle); };
    void addClassPermenant(const std::string &name) { add(name, BLWindowClass); };
    void addTitlePermenant(const std::string &name) { add(name, BLWindowTitle); };

    inline bool hasClass(const std::string& item) { return exists(item, BLWindowClass) || exists(item, temp_BLWindowClass); }
    inline bool hasTitle(const std::string& item) { return exists(item, BLWindowTitle) || exists(item, temp_BLWindowTitle); }

private:
    bool exists(const std::string& item, const Json::Value::Members& list);
    bool coexists(const std::string& item, const Json::Value::Members& list);
    void add(const std::string& item, std::vector<std::string>& list);
    void remove(const std::string& item, Json::Value::Members& list);

    void parseBlockList();
    void updateBlockList();

    void write_default();
};

extern _BlockList BlockList;
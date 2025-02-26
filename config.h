#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <json/json.h>

class _Config {
private:
    Json::Value root;
    std::string filename = "./config.json";

public:
    // --Functions
    void setFileName(std::string& _filename);
    
    void load(); //load from file
    void save(); //save to file

private:
    void parse();
    void compile();

public:
    // --Values
    bool EnableBlockList = true;
    size_t MaxPathLength = 260;
    struct {
        bool AutoUpdateEnabled = true;
        int AutoUpdateInterval = 1000;
    } PropertyWindow;
};

extern _Config Config;
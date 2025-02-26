#include "config.h"

_Config Config;

void _Config::setFileName(std::string& _filename) {
    filename = _filename;
}

void _Config::load() {
    std::ifstream file(filename);
    if(file.bad()) save();
    file.open(filename);
    file >> root;
    file.close();
    parse();
}

void _Config::save() {
    compile();
    std::ofstream file(filename);
    file << root;
    file.close();
}

void _Config::parse() {
    if(root.isMember("EnableBlockList")) {
        EnableBlockList = root["EnableBlockList"].asBool();
    }

    if(root.isMember("MaxPathLength")) {
        MaxPathLength = root["MaxPathLength"].asInt64();
    }

    if(root.isMember("PropertyWindow")) {
        PropertyWindow.AutoUpdateEnabled = root["PropertyWindow"]["AutoUpdateEnabled"].asBool();
        PropertyWindow.AutoUpdateInterval = root["PropertyWindow"]["AutoUpdateInterval"].asInt();
    }

}

void _Config::compile() {
    root.clear();
    root["EnableBlockList"] = EnableBlockList;
    root["MaxPathLength"] = MaxPathLength;
    root["PropertyWindow"]["AutoUpdateEnabled"] = PropertyWindow.AutoUpdateEnabled;
    root["PropertyWindow"]["AutoUpdateInterval"] = PropertyWindow.AutoUpdateInterval;
}
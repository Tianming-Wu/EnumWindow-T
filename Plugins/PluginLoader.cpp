/*
    PluginLoader 在这样一个分离的 dll 文件中，不在主程序内直接调用，以免被杀毒软件视为攻击性行为。
    主程序会手动加载此 dll 并且获取对应的函数指针来加载插件。
*/


#include <string>
#include <thread>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;
#include <optional>
#include <fstream>


#include <windows.h>
#include <psapi.h>

std::string TranslateLastError() {
    DWORD errorCode = GetLastError();
    if (errorCode == 0) return ""; // 成功，没有错误消息。
    LPSTR messageBuffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr
    );

    std::string result;
    if (size > 0 && messageBuffer != nullptr) {
        result.assign(messageBuffer, size);
        while (!result.empty() &&  (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
    } else result = "";
    if (messageBuffer != nullptr) LocalFree(messageBuffer);
    return result;
}

fs::path getExecutableDir() {
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) != 0) {
        std::string pathStr = std::string(exePath);
        size_t lastSlash = pathStr.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return pathStr.substr(0, lastSlash);
        }
    }
    return ""; // 获取路径失败，返回空字符串。
}

enum class ParamType : uint8_t {
    Unknown, Int, Float, String, Bool, HWND
};

enum class ParamExType : uint8_t {
    None, Auto, Hidden, Reserved
};

struct PluginMetaData {
    std::string Name, Author, Description, Version;
    int ParamCount;

    bool entrySupportVerified = false; // 是否已经验证过插件导出 PluginMain 函数

    struct ParamMeta {
        std::string Name, DefaultValue, Description;
        ParamType Type;
        ParamExType ExType;
    };
    std::vector<ParamMeta> Params;
};

enum class PluginType : uint8_t {
    Generic, Specialized
};

struct PluginData {
    PluginType type = PluginType::Generic;
    std::optional<PluginMetaData> MetaData;
    fs::path fullPath;
};

struct PluginLoad {
    PluginData data;
    std::vector<std::string> paramValues;
};

std::vector<PluginData> availablePlugins;

size_t loadParam(ParamType type, const std::string& valueStr, char* buffer, size_t& top) {
    switch(type) {
        case ParamType::Int:
            *(int*)(buffer + top) = std::stoi(valueStr);
            top += sizeof(int);
            break;
        case ParamType::Float:
            *(float*)(buffer + top) = std::stof(valueStr);
            top += sizeof(float);
            break;
        case ParamType::String: {
            // 对于字符串类型，需要额外申请 RemoteMem 写入，然后把指针放到这里。
            // 内容由上一级负责，此处仅写入上一级传入的指针（已为远程地址），从 valueStr 传入。
            uintptr_t remoteStrAddr = std::stoull(valueStr, nullptr, 16);
            *(uintptr_t*)(buffer + top) = remoteStrAddr;
            break;
        }
        case ParamType::Bool:
            *(bool*)(buffer + top) = (valueStr == "true");
            top += sizeof(bool);
            break;
        case ParamType::HWND:
            *(HWND*)(buffer + top) = (HWND)(uintptr_t)std::stoul(valueStr, nullptr, 16);
            top += sizeof(HWND);
            break;
        case ParamType::Unknown:
        default:
            return 0; // 不支持的参数类型
    }
    return 1; // 成功加载参数
}

ParamType parseParamType(const std::string& typeStr) {
    if (typeStr == "int") return ParamType::Int;
    if (typeStr == "float") return ParamType::Float;
    if (typeStr == "string") return ParamType::String;
    if (typeStr == "bool") return ParamType::Bool;
    if (typeStr == "hwnd") return ParamType::HWND;
    return ParamType::Unknown;
}

size_t getParamSize(const std::vector<PluginMetaData::ParamMeta>& Params) {
    size_t totalSize = 0;
    for (const auto& param : Params) {
        if (param.Type == ParamType::Int) totalSize += sizeof(int);
        else if (param.Type == ParamType::Float) totalSize += sizeof(float);
        else if (param.Type == ParamType::Bool) totalSize += sizeof(bool);
        else if (param.Type == ParamType::HWND) totalSize += sizeof(HWND);
        else if (param.Type == ParamType::String) totalSize += 256; // 假设字符串参数最大长度为 256
    }
    return totalSize;
}

void checkPluginEntrySupport(PluginData& pluginData) {
    // 检查插件是否导出 PluginMain 函数，但是不加载插件。

    HMODULE hLocalDll = LoadLibraryExA(pluginData.fullPath.string().c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
    if(!hLocalDll) {
        return; // 无法加载，视为无效
    }

    FARPROC pPluginMain = GetProcAddress(hLocalDll, "PluginMain");
    if(!pPluginMain) {
        FreeLibrary(hLocalDll);
        return; // 没有导出 PluginMain，视为不支持
    }

    FreeLibrary(hLocalDll);
    pluginData.MetaData->entrySupportVerified = true;
}

int injectPlugin(DWORD pid, const PluginLoad& pluginLoad) {
    const auto& pluginData = pluginLoad.data;
    const auto& paramValues = pluginLoad.paramValues;
    HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process_handle) {
        MessageBoxA(NULL, ("OpenProcess failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    size_t dll_path_len = pluginData.fullPath.string().length() + 1;

    LPVOID remote_mem = VirtualAllocEx(process_handle, NULL, dll_path_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        CloseHandle(process_handle);
        MessageBoxA(NULL, ("VirtualAllocEx failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    WriteProcessMemory(process_handle, remote_mem, pluginData.fullPath.string().c_str(), dll_path_len, NULL);

    HANDLE thread_handle = CreateRemoteThread(
        process_handle,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        remote_mem,
        0,
        NULL
    );

    if (thread_handle) {
        if(WaitForSingleObject(thread_handle, 5000) != WAIT_OBJECT_0) {
            MessageBoxA(NULL, "Remote thread did not complete in 5 seconds, exited forcefully.", "Warning", MB_OK | MB_ICONWARNING);
        }
        CloseHandle(thread_handle);
    } else {
        MessageBoxA(NULL, ("CreateRemoteThread failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
    }

    VirtualFreeEx(process_handle, remote_mem, 0, MEM_RELEASE);

    if(thread_handle == NULL) {
        CloseHandle(process_handle);
        return FALSE;
    }

    // 从这里开始，通用插件和专用插件的处理会发生差异

    if (pluginData.type == PluginType::Generic) {
        CloseHandle(process_handle);
        return TRUE; // 对于通用插件，我们不需要做额外的事情。
    }

    if (!pluginData.MetaData.has_value()) {
        MessageBoxA(NULL, "Plugin metadata is missing for a specialized plugin.", "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // 获取远程 dll 的地址
    HMODULE hRemoteDLL = NULL;
    while (hRemoteDLL == NULL) {
        HMODULE hModules[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(process_handle, hModules, sizeof(hModules), &cbNeeded)) {
            size_t moduleCount = cbNeeded / sizeof(HMODULE);
            for (size_t i = 0; i < moduleCount; ++i) {
                char modulePath[MAX_PATH];
                if (GetModuleFileNameExA(process_handle, hModules[i], modulePath, sizeof(modulePath))) {
                    if (pluginData.fullPath.filename() == fs::path(modulePath).filename()) {
                        hRemoteDLL = hModules[i];
                        break;
                    }
                }
            }
        }
        if (hRemoteDLL == NULL) Sleep(100); // 等待远程 DLL 加载完成
    }

    // 这里需要尝试获取 PluginMain 的地址，并由此计算出它在远程进程中的地址。
    HMODULE hLocalDll = LoadLibraryExA(pluginData.fullPath.string().c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
    if(!hLocalDll) {
        MessageBoxA(NULL, ("Failed to load local copy of plugin DLL: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    FARPROC pPluginMain = GetProcAddress(hLocalDll, "PluginMain");
    if(!pPluginMain) {
        FreeLibrary(hLocalDll);
        MessageBoxA(NULL, "PluginMain entry point not found in plugin DLL.", "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    size_t rvaFunc = (size_t)pPluginMain - (size_t)hLocalDll;
    size_t remotePluginMain = (size_t)hRemoteDLL + rvaFunc;

    HANDLE hRemoteThread = NULL;

    const auto& metaData = pluginData.MetaData.value();

    if(metaData.Params.empty()) {
        // 空参数，调用 PluginMain(NULL).
        hRemoteThread = CreateRemoteThread(process_handle, NULL, 0, (LPTHREAD_START_ROUTINE)remotePluginMain, NULL, 0, NULL);
    } else {
        // 根据参数列表构造参数结构体，写入进程内容，并传递给 PluginMain

        HANDLE hRemoteMem = VirtualAllocEx(process_handle, NULL, getParamSize(metaData.Params), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!hRemoteMem) {
            FreeLibrary(hLocalDll);
            CloseHandle(process_handle);
            MessageBoxA(NULL, ("VirtualAllocEx for parameters failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
        char* buffer = new char[getParamSize(metaData.Params)];
        size_t top = 0, paramTop = 0;

        for (const auto& param : metaData.Params) {
            if(param.Type == ParamType::String) {
                std::string paramValue = paramValues[paramTop++];
                
                HANDLE hRemoteStr = VirtualAllocEx(process_handle, NULL, paramValue.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!hRemoteStr) {
                    delete[] buffer;
                    VirtualFreeEx(process_handle, hRemoteMem, 0, MEM_RELEASE);
                    FreeLibrary(hLocalDll);
                    CloseHandle(process_handle);
                    MessageBoxA(NULL, ("VirtualAllocEx for string parameter failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
                    return FALSE;
                }
                
                // 把字符串参数写入远程内存
                WriteProcessMemory(process_handle, hRemoteStr, paramValue.c_str(), paramValue.size() + 1, NULL);

                // 把远程字符串地址写入参数结构体
                *(uintptr_t*)(buffer + top) = (uintptr_t)hRemoteStr;
                top += sizeof(uintptr_t);

            } else {
                // 其他类型可以普通加载
                loadParam(param.Type, paramValues[paramTop++], (char*)buffer, top);
            }
        }

        // 将参数结构体写入远程内存
        WriteProcessMemory(process_handle, hRemoteMem, buffer, top, NULL);
        delete[] buffer;


        hRemoteThread = CreateRemoteThread(process_handle, NULL, 0, (LPTHREAD_START_ROUTINE)remotePluginMain, hRemoteMem, 0, NULL);
    }

    if (!hRemoteThread) {
        FreeLibrary(hLocalDll);
        CloseHandle(process_handle);
        MessageBoxA(NULL, ("CreateRemoteThread for PluginMain failed: " + TranslateLastError()).c_str(), "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // 不应等待该线程，插件加载已经完成。
    FreeLibrary(hLocalDll);
    return TRUE;
}

int loadPlugin_impl(DWORD pid, const std::string& plugin_name) {
    // 检查插件是否存在
    



    return 0;
}

// 不使用 WinMain 函数
extern "C" __declspec(dllexport) int loadPlugin(DWORD pid, const std::string& plugin_name) {
    return loadPlugin_impl(pid, plugin_name);
}

extern "C" __declspec(dllexport) int showPluginWindow(HWND targetHwnd) {
    // 这个函数需要创建注入选择窗口，以便用户选择要注入的插件。

    return 0;
}

extern "C" __declspec(dllexport) int initPluginSystem() {
    // 这个函数需要尝试加载 Plugins 目录下的插件，识别它们的 MetaData 文件。

    std::filesystem::path pluginsDir = getExecutableDir() / "Plugins";
    if (!std::filesystem::exists(pluginsDir) || !std::filesystem::is_directory(pluginsDir)) {
        MessageBoxA(NULL, ("Plugins directory not found: " + pluginsDir.string()).c_str(), "Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    for(const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
        if (entry.is_directory()) { // 只扫描一级目录，插件应该放在以插件名命名的子目录里。
            const fs::path& pluginDir = entry.path();

            for(const auto& fileEntry : std::filesystem::directory_iterator(pluginDir)) {
                if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".dll") {
                    PluginData pluginData;
                    pluginData.fullPath = fileEntry.path();

                    // 仅在 meta 文件存在时才尝试加载，否则视作通用插件。
                    fs::path metaFilePath = pluginDir / (pluginDir.filename().string() + ".meta");
                    if (fs::exists(metaFilePath)) {
                        // 解析 Meta 文件，稍后引入一个轻量 ini 库。
                        PluginMetaData metaData;

                    }

                    availablePlugins.push_back(std::move(pluginData));
                }
            }
        }
    }

    return 0;
}

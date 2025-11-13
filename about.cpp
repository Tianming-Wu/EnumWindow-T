#include "about.hpp"
#include <windows.h>

namespace about {

void showAbout() {
    MessageBox(NULL,
R"(WindowControl (EnumWindow-T)
项目地址: https://github.com/Tianming-Wu/EnumWindow-T

Author: Tianming Wu (c)2025

本项目使用的是 MIT 许可证。
本项目基于 Windows, jsoncpp.

jsoncpp: https://github.com/open-source-parsers/jsoncpp
)",
    "关于", MB_OK);
}

void showHelp() {
    DWORD result = MessageBox(NULL, "需要打开帮助文档（在线）吗？", "帮助", MB_YESNO);
    if(result == IDYES) {
        ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/Tianming-Wu/EnumWindow-T/wiki"), NULL, NULL, SW_SHOWNORMAL);
    }
}

} // namespace about
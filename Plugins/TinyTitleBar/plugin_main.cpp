/*
    TinyTitleBar
    这个插件的功能是为窗口添加一个小标题栏。适用于一些未合理适配的信息窗口。
    例如 PowerToys.CropAndLock 的裁剪窗口，如果尝试用它显示一些较小的信息窗口，标题栏会占用太多空间，导致
    有效信息的比例很低，并且会遮挡屏幕内的其他内容。

    可以通过插件dll所在目录，或目标可执行文件所在目录（优先）下的 “目标可执行文件名.ttb.config” 文件调整标题栏的高度。（暂未实现）
*/

#include <windows.h>
#include <thread>
#include <string>

#include "plugin.hpp"

CLAIM_PLUGIN_ENTRYSUPPORT

HMODULE g_hModule;
WNDPROC g_originalWndProc;

int titleBarHeight = 0; // 0 is default, will be changed to the default value later.

LRESULT CALLBACK CustomWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

PLUGIN_ENTRY void PluginMain(void* lpParam) {
    HWND m_hTargetWnd = (HWND)lpParam;

    // 必须有效
    if (m_hTargetWnd == NULL) {
        MessageBoxA(NULL, "PluginMain获取到了无效的窗口句柄。", "TinyTitleBar", MB_OK | MB_ICONERROR);
        return;
    }

    // 检查配置文件是否存在，获取可执行文件名失败视为不存在（虽然几乎不可能发生）
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) != 0) {
        std::string exeName = std::string(exePath);
        size_t lastSlash = exeName.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            exeName = exeName.substr(lastSlash + 1);
        }
    }





    // 移除窗口现有的标题栏
    LONG style = GetWindowLong(m_hTargetWnd, GWL_STYLE);
    style &= ~WS_CAPTION; // 移除标题栏样式
    SetWindowLong(m_hTargetWnd, GWL_STYLE, style);

    // 注入 WindowProc 钩子，以实现绘制自定义标题栏、处理拖动等功能
    g_originalWndProc = (WNDPROC)SetWindowLongPtr(m_hTargetWnd, GWLP_WNDPROC, (LONG_PTR)CustomWindowProc);

    // 重新绘制窗口以应用更改
    SetWindowPos(m_hTargetWnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 注入到此完成，插件的入口不需要保持运行
}

LRESULT CALLBACK CustomWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{






    // Call the original window procedure for default processing
    return g_originalWndProc(hwnd, uMsg, wParam, lParam);
}
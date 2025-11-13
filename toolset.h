#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <map>

#define DTS_DEFAULT (DT_LEFT|DT_SINGLELINE|DT_NOCLIP)

/// @brief Toolset header
/// @attention original TextOut() only support length as int, so content may be missing.
BOOL TextOutStd(HDC _hdc, int x, int y, std::string _str);
int DrawTextStd(HDC hdc, std::string Text, LPRECT lprc, UINT format, bool Repaint = false);

RECT makeRectAbsolute(int t, int b, int l, int r);
RECT makeRectRelative(int x, int y, int h, int w);
RECT posRect(int x, int y);

void MakeSizeGrip(int* _array, int Size);

HWND GetItemHandle(HWND hTreeView);

bool SetMenuText(HMENU hMenu, int uid, std::string Text);
void SetMenuSelection(HMENU hMenu, int range_l, int range_r, int active);

std::string GetWindowClass(HWND hwnd);
std::string _GetWindowText(HWND hwnd);

template<typename _T> [[deprecated]]
    std::string _tConv(_T _data) {
        std::stringstream sstr;
        sstr << _data;
        return sstr.str();
    }

enum SubType {
    ST_Property, ST_FindWindow, ST_SearchWindow, ST_OutlineWindow
};

struct subThreadItem {
    std::thread* handle;
    SubType type;
    HWND hwnd;
};

struct subWindowItem {
    HWND hwnd;
    SubType type;
    subWindowItem(HWND hwnd) : hwnd(hwnd) {}
    subWindowItem(HWND hwnd, SubType type) : hwnd(hwnd), type(type) {}
};

extern std::vector<subThreadItem> subThreads;
extern std::vector<subWindowItem> subWindows;


void CloseAllSubWindows();
// 等待所有子线程退出
void CloseAllThreads();
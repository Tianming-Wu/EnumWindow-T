#ifndef WCEX_H
#define WCEX_H

#include <windows.h>
#include <commctrl.h>

#include <thread>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

#include "menu_items.h"
#include "toolset.h"
#include "blocklist_fx.h"

#define WM_INITIALIZE WM_USER+7

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HTREEITEM AddChildItem(HWND hwndTree, HTREEITEM hParent, LPCTSTR lpszItem, LPARAM lParam);
HTREEITEM AddRootItem(HWND hwndTree, LPCTSTR lpszItem, LPARAM lParam);

#endif
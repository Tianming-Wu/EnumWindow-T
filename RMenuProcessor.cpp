/*
    This file handles the right click menu and the static menubar.
*/
#pragma once
#include "RMenuProcessor.hpp"

#include <windows.h>
#include <CommCtrl.h>
#include <functional>

#include "menu_items.h"
#include "config.h"
#include "RuleSet.hpp"

#include "about.hpp"

#include "PropertyWindow.hpp"
#include "TargetWindow.hpp"
#include "SearchWindow.hpp"
#include "PublicDefs.hpp"

#include <shellapi.h>

//#include "WindowOperations.hpp"
#include "ScanThread.hpp"
#include "toolset.h"

extern HMENU hWindowMenu;
extern HWND hTreeView;

extern bool Scanning;

int EmitConfigChangeSignal()
{
    for(auto &i: subWindows) {
        if(i.type == ST_Property && IsWindow(i.hwnd))
            SendMessage(i.hwnd, WM_PW_CONFIGUPD, 0, 0);
    }
    return 0;
}

LRESULT CALLBACK RMenuProcessor(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    // ------ 右键菜单 ------
    case IDW_PROPERTY: // 这一条是菜单栏的，逻辑完全一致所以复用
    case IDM_PROPERTY:
    {
        HWND hwndHandle;
        if((hwndHandle = GetItemHandle(hTreeView)) != nullptr) {
            if (IsWindow(hwndHandle)) {
                StartPropertyWindow(hwndHandle);
            } else {
                MessageBox(hwnd, "窗口已经失效。", "信息", MB_ICONINFORMATION);
            }
        }
        else MessageBox(hwnd, "无法从列表项提取窗口句柄。", "错误", MB_ICONERROR);
        break;
    }
    case IDM_SHOWPOSITION:
        //ShowWindowOutline(GetItemHandle(hTreeView)); // 这个子模块不工作，稍后尝试修复
        break;
    case IDM_SWITCHTO:
        SetForegroundWindow(GetItemHandle(hTreeView));
        break;
    case IDM_MOVETOCENTER:
        HWND tHwnd;
        if(IsWindow(tHwnd = GetItemHandle(hTreeView))) {
            RECT tWndRect, monRect;
            GetWindowRect(tHwnd, &tWndRect);
            SystemParametersInfo(SPI_GETWORKAREA, 0, &monRect, 0);
            int wWidth = tWndRect.right - tWndRect.left, wHeight = tWndRect.bottom - tWndRect.top;
            int screenWidth = monRect.right - monRect.left, screenHeight = monRect.bottom - monRect.top;
            int x = monRect.left + (screenWidth - wWidth) / 2, y = monRect.top + (screenHeight - wHeight) / 2;
            MoveWindow(tHwnd, x, y, wWidth, wHeight, false);
        } else MessageBox(hwnd, "未选择窗口", "警告", MB_ICONHAND);
        break;
    case IDM_SHOW_OTHER_WINDOWS:
        break;
    case IDM_EXPAND_ALL:
    case IDM_COLLAPSE_ALL:
    {
        std::function<void(HTREEITEM, UINT)> wtP_ = 
            [&](HTREEITEM hItem, UINT flag) {
                if(hItem == NULL) return;
                HTREEITEM hChild = TreeView_GetChild(hTreeView, hItem);
                while (hChild != NULL)
                {
                    wtP_(hChild, flag);
                    hChild = TreeView_GetNextSibling(hTreeView, hChild);
                }
                TreeView_Expand(hTreeView, hItem, flag);
            };
        HTREEITEM hFocusedItem = TreeView_GetNextItem(hTreeView, NULL, TVGN_CARET);
        if(hFocusedItem == NULL) hFocusedItem = TreeView_GetRoot(hTreeView);
        if(hFocusedItem != NULL)
            wtP_(hFocusedItem, (LOWORD(wParam)==IDM_EXPAND_ALL)?(TVE_EXPAND):(TVE_COLLAPSE));
        break;
    }

    case IDM_MINIMIZE:
        ShowWindow(GetItemHandle(hTreeView), SW_MINIMIZE); break;
    case IDM_MAXIMIZE:
        ShowWindow(GetItemHandle(hTreeView), SW_MAXIMIZE); break;
    case IDM_RESTORE:
        ShowWindow(GetItemHandle(hTreeView), SW_RESTORE); break;
    case IDM_CLOSE:
        CloseWindow(GetItemHandle(hTreeView)); break;

    case IDM_IGNORE_CLASS_SINGLETIME:
        RuleSet.insertTempRule(GetWindowClass(GetItemHandle(hTreeView)), PatternKey::Class); break; // BeginScan(); ? 或者尝试增量移除此窗口。
    case IDM_IGNORE_CLASS_PERMANENT:
        RuleSet.insertRule(GetWindowClass(GetItemHandle(hTreeView)), PatternKey::Class); break;
    case IDM_IGNORE_TITLE_SINGLETIME:
        RuleSet.insertTempRule(_GetWindowText(GetItemHandle(hTreeView)), PatternKey::Title); break;
    case IDM_IGNORE_TITLE_PERMANENT:
        RuleSet.insertRule(_GetWindowText(GetItemHandle(hTreeView)), PatternKey::Title); break;
    

    // ------ 菜单栏 ------
    case IDW_EXITPROGRAM: // 退出程序
        DestroyWindow(hwnd); break;

    case IDW_EXPAND_ALL:
    case IDW_COLLAPSE_ALL:
    {
        std::function<void(HTREEITEM, UINT)> wtP_ = 
            [&](HTREEITEM hItem, UINT flag) {
                HTREEITEM hChild = TreeView_GetChild(hTreeView, hItem);
                while (hChild != NULL)
                {
                    wtP_(hChild, flag);
                    hChild = TreeView_GetNextSibling(hTreeView, hChild);
                }
                TreeView_Expand(hTreeView, hItem, flag);
            };
        // 遍历所有根节点，分别进行展开/折叠
        HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
        if(hRoot != NULL) {
            HTREEITEM cur = hRoot;
            while(cur != NULL) {
                wtP_(cur, (LOWORD(wParam)==IDW_EXPAND_ALL)?(TVE_EXPAND):(TVE_COLLAPSE));
                cur = TreeView_GetNextSibling(hTreeView, cur);
            }
        } else {
            MessageBox(hwnd, "错误: hRoot is NULL", "错误", MB_ICONHAND|MB_OK);
        }
        break;
    }

    case IDW_BLACKLIST_SETTINGS:
        break;
    case IDW_BLACKLIST_RESET: {
        int ConfirmResult = MessageBox(hwnd, "确定要重置黑名单吗？这将清除所有自定义规则。", "确认", MB_ICONQUESTION | MB_YESNO);
        if(ConfirmResult == IDYES) {
            RuleSet.resetRuleSet();
        }
        break;
    }
    case IDW_BLACKLIST_ENABLED:
        Config.EnableBlockList = ! Config.EnableBlockList;
        CheckMenuItem(hWindowMenu, IDW_BLACKLIST_ENABLED, Config.EnableBlockList?(MF_CHECKED):(MF_UNCHECKED));
        break;
    
    case IDW_TARGETWINDOW:
        StartTargetWindow();
        break;
    case IDW_SEARCHWINDOW:
        StartSearchWindow();
        break;
    case IDW_ABOUT:
        about::showAbout();
        break;
    case IDW_HELP:
        about::showHelp();
        break;
    case IDW_REFRESHLIST:
        BeginScan();
        break;
    case IDW_ELEVATE:
    {
        CHAR path[MAX_PATH] = {0};
        if(GetModuleFileNameA(NULL, path, MAX_PATH)) {
            HINSTANCE r = ShellExecuteA(NULL, "runas", path, NULL, NULL, SW_SHOWNORMAL);
            if((INT_PTR)r > 32) {
                HWND mainWnd = GetAncestor(hTreeView, GA_ROOT);
                if(mainWnd) PostMessage(mainWnd, WM_CLOSE, 0, 0);
                else ExitProcess(0);
            } else {
                MessageBox(hwnd, "提权被取消或失败", "信息", MB_ICONINFORMATION | MB_OK);
            }
        }
        break;
    }
    case IDW_AUTOUPDATE:
    {
        Config.PropertyWindow.AutoUpdateEnabled = ! Config.PropertyWindow.AutoUpdateEnabled;
        CheckMenuItem(hWindowMenu, IDW_AUTOUPDATE, Config.PropertyWindow.AutoUpdateEnabled?(MF_CHECKED):(MF_UNCHECKED));
        HMENU hSubMenu = GetSubMenu(hWindowMenu, 4);
        EnableMenuItem(hSubMenu, 3, MF_BYPOSITION | (Config.PropertyWindow.AutoUpdateEnabled?(MF_ENABLED):(MF_DISABLED)) );

        EmitConfigChangeSignal();
        break;
    }
    case IDW_AUINTERVAL50MS  :
        Config.PropertyWindow.AutoUpdateInterval =   50;
        SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, "...");
        SetMenuSelection(hWindowMenu, IDW_AUINTERVAL50MS, IDW_AUINTERVALCUSTOM,   IDW_AUINTERVAL50MS);
        EmitConfigChangeSignal();
        break;
    case IDW_AUINTERVAL100MS :
        Config.PropertyWindow.AutoUpdateInterval =  100;
        SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, "...");
        SetMenuSelection(hWindowMenu, IDW_AUINTERVAL50MS, IDW_AUINTERVALCUSTOM,  IDW_AUINTERVAL100MS);
        EmitConfigChangeSignal();
        break;
    case IDW_AUINTERVAL500MS :
        Config.PropertyWindow.AutoUpdateInterval =  500;
        SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, "...");
        SetMenuSelection(hWindowMenu, IDW_AUINTERVAL50MS, IDW_AUINTERVALCUSTOM,  IDW_AUINTERVAL500MS);
        EmitConfigChangeSignal();
        break;
    case IDW_AUINTERVAL1000MS:
        Config.PropertyWindow.AutoUpdateInterval = 1000;
        SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, "...");
        SetMenuSelection(hWindowMenu, IDW_AUINTERVAL50MS, IDW_AUINTERVALCUSTOM, IDW_AUINTERVAL1000MS);
        EmitConfigChangeSignal();
        break;
    case IDW_AUINTERVAL5000MS:
        Config.PropertyWindow.AutoUpdateInterval = 5000;
        SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, "...");
        SetMenuSelection(hWindowMenu, IDW_AUINTERVAL50MS, IDW_AUINTERVALCUSTOM, IDW_AUINTERVAL5000MS);
        EmitConfigChangeSignal();
        break;
    case IDW_AUINTERVALCUSTOM: {
        // 这里本来应该弹窗问用户自定义延时的大小，但是我没有找到简单的解决方案
        MessageBox(hwnd, "请手动更改配置文件:\nconfig.json:\n    PropertyWindow : AutoUpdateInterval", "信息", MB_ICONINFORMATION|MB_OK);

        // Config.PropertyWindow.AutoUpdateInterval = Get();

        // std::string p = std::to_string(Config.PropertyWindow.AutoUpdateInterval) + "ms";
        // SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, p);
        // DrawMenuBar(hwnd);
        break;
    }
    }
    return 0;
}
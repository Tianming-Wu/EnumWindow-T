/*
    This file handles the right click menu and the static menubar.
*/
#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <functional>

#include "menu_items.h"
#include "config.h"
#include "blocklist_fx.h"

#include "PropertyWindow.hpp"
#include "FindWindow.hpp"
#include "SearchWindow.hpp"
#include "PublicDefs.hpp"

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
        if((hwndHandle = GetItemHandle(hTreeView)) != nullptr)
            StartPropertyWindow(hwndHandle);
        else MessageBox(hwnd, "Failed to fetch data.", "ERROR", MB_ICONERROR);
        break;
    }
    case IDM_SHOWPOSITION:
        //ShowWindowOutline(GetItemHandle(hTreeView)); //Not working
        break;
    case IDM_SWITCHTO:
        SetForegroundWindow(GetItemHandle(hTreeView));
        break;
    case IDM_MOVETOCENTER: // GRAYED
        HWND tHwnd;
        if(IsWindow(tHwnd = GetItemHandle(hTreeView))) {
            RECT tWndRect, monRect;
            GetWindowRect(tHwnd, &tWndRect);
            GetClientRect(NULL, &monRect);
            int height = tWndRect.bottom - tWndRect.top, width = tWndRect.right - tWndRect.left;
            MoveWindow(tHwnd, (monRect.bottom - height) / 2, (monRect.right - width) / 2, width, height, false);
        } else MessageBox(hwnd, "No Window selected", "WARNING", MB_ICONHAND);
        break;
    case IDM_SHOW_OTHER_WINDOWS:
        break;
    case IDM_EXPAND_ALL:
    case IDM_COLLAPSE_ALL:
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
        HTREEITEM hFocusedItem = TreeView_GetNextItem(hTreeView, NULL, TVGN_CARET);
        wtP_(hFocusedItem, (LOWORD(wParam)==IDM_EXPAND_ALL)?(TVE_EXPAND):(TVE_COLLAPSE));
        break;
    }
    case IDM_IGNORE_CLASS_SINGLETIME:
        BlockList.addClass(GetWindowClass(GetItemHandle(hTreeView))); break;
    case IDM_IGNORE_CLASS_PERMANENT:
        BlockList.addClassPermenant(GetWindowClass(GetItemHandle(hTreeView))); break;
    case IDM_IGNORE_TITLE_SINGLETIME:
        BlockList.addTitle(_GetWindowText(GetItemHandle(hTreeView))); break;
    case IDM_IGNORE_TITLE_PERMANENT:
        BlockList.addTitlePermenant(_GetWindowText(GetItemHandle(hTreeView))); break;
    

    // ------ 菜单栏 ------
    case IDW_EXITPROGRAM:
        DestroyWindow(hwnd);
        break;

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
        // HTREEITEM hFocusedItem = TreeView_GetNextItem(hTreeView, NULL, TVGN_CARET);
        HTREEITEM hRoot = TreeView_GetRoot(hTreeView);
        if(hRoot != NULL) {
            wtP_(hRoot, (LOWORD(wParam)==IDW_EXPAND_ALL)?(TVE_EXPAND):(TVE_COLLAPSE));
        } else {
            MessageBox(hwnd, "ERROR: hRoot is NULL", "ERROR", MB_ICONHAND|MB_OK);
        }
        break;
    }

    case IDW_BLACKLIST_ENABLED:
        Config.EnableBlockList = ! Config.EnableBlockList;
        CheckMenuItem(hWindowMenu, IDW_BLACKLIST_ENABLED, Config.EnableBlockList?(MF_CHECKED):(MF_UNCHECKED));
        break;
    
    case IDW_FINDWINDOW:
        StartFindWindow();
        break;
    case IDW_SEARCHWINDOW:
        StartSearchWindow();
        break;
    case IDW_ABOUT:
        // 显示关于页面（未制作）
        break;
    case IDW_HELP:
        // 显示帮助页面（未制作）
        break;
    case IDW_REFRESHLIST:
        BeginScan();
        break;
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
#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <functional>

#include "menu_items.h"
#include "config.h"
#include "blocklist_fx.h"

#include "PropertyWindow.hpp"
#include "FindWindow.hpp"
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
    // RIGHT_MENU
    case IDW_PROPERTY:
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

    // WINDOW MENU
    case IDW_EXITPROGRAM:
        DestroyWindow(hwnd);
        break;
    case IDW_BLACKLIST_ENABLED:
        Config.EnableBlockList = ! Config.EnableBlockList;
        CheckMenuItem(hWindowMenu, IDW_BLACKLIST_ENABLED, Config.EnableBlockList?(MF_CHECKED):(MF_UNCHECKED));
        break;
    
    case IDW_FINDWINDOW:
        StartFindWindow();
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
        MessageBox(hwnd, "请手动更改配置文件:\nconfig.json:\n PropertyWindow : AutoUpdateInterval", "信息", MB_ICONINFORMATION|MB_OK);

        // Config.PropertyWindow.AutoUpdateInterval = Get();

        // std::string p = std::to_string(Config.PropertyWindow.AutoUpdateInterval) + "ms";
        // SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, p);
        // DrawMenuBar(hwnd);
        break;
    }
    }
    return 0;
}
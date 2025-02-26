#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <thread>

#include "blocklist_fx.h"
#include "config.h"

extern bool Scanning;
extern std::thread *scanthrd;
extern int windowcount, filteredwindowcount;
extern HWND main_hwnd, _hStatusBar;
extern HTREEITEM tree_conduct;

bool notFirstScan;

#define WM_ADDROOT WM_USER+1
#define WM_ADDCHILD WM_USER+2
#define WM_RESET WM_USER+3

struct MsgDataPackA { //for root
    std::string Name;
    HWND handle;
    HANDLE event;
public: MsgDataPackA(std::string _name,HWND _handle,HANDLE _event):Name(_name),handle(_handle),event(_event){};
};

struct MsgDataPackB { //for child
    std::string Name;
    HWND handle;
    HTREEITEM parent;
    HANDLE event;
public: MsgDataPackB(std::string _name,HWND _handle,HTREEITEM _parent,HANDLE _event):Name(_name),handle(_handle),parent(_parent),event(_event){};
};

void EnumSubWindows(HTREEITEM tree, HWND hwnd)
{
    char className[256], windowTitle[256];
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowTitle, sizeof(windowTitle));

    HANDLE updevent = CreateEventA(NULL, FALSE, FALSE, NULL);

    std::stringstream sstr; std::string tstr;
    sstr << std::hex<< hwnd; tstr = sstr.str(); sstr.str("");
    tstr = tstr.substr(tstr.length()-6);
    sstr << "0x" << tstr << " | " << className;
    if(!std::string(windowTitle).empty()) sstr << " | " << windowTitle;
    MsgDataPackB datapack(sstr.str(), hwnd, tree, updevent);
    PostMessage(main_hwnd, WM_ADDCHILD, 0, reinterpret_cast<LPARAM>(&datapack));

    DWORD dwWaitResult = WaitForSingleObject(updevent, INFINITE);
    HTREEITEM htree = tree_conduct;

    CloseHandle(updevent);

    HWND child = GetWindow(hwnd, GW_CHILD);
    while (child != NULL)
    {
        EnumSubWindows(htree, child);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    char className[256], windowTitle[256];
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowTitle, sizeof(windowTitle));

    if ( !Config.EnableBlockList || ((!BlockList.hasClass(className)) && (!BlockList.hasTitle(windowTitle))) ) {
        HANDLE updevent = CreateEventA(NULL, FALSE, FALSE, NULL);

        std::stringstream sstr; std::string tstr;
        sstr << std::hex<< hwnd; tstr = sstr.str(); sstr.str("");
        tstr = tstr.substr(tstr.length()-6);
        sstr << "0x" << tstr << " | " << className;
        if(!std::string(windowTitle).empty()) sstr << " | " << windowTitle;
        MsgDataPackA datapack(sstr.str(), hwnd, updevent);
        PostMessage(main_hwnd, WM_ADDROOT, 0, reinterpret_cast<LPARAM>(&datapack));

        DWORD dwWaitResult = WaitForSingleObject(updevent, INFINITE);
        HTREEITEM htree = tree_conduct;

        CloseHandle(updevent);

        HWND child = GetWindow(hwnd, GW_CHILD);
        while (child != NULL)
        {
            EnumSubWindows(htree, child);
            child = GetWindow(child, GW_HWNDNEXT);
        }

    } else filteredwindowcount++;
    
    windowcount++;

    return TRUE;
}

int ScanThread() {
    Scanning = true;
    EnumWindows(EnumWindowsProc, 0);

    std::stringstream sstr;
    sstr << "窗口数：(" << windowcount-filteredwindowcount << "/" << windowcount << ")";
    SendMessage(_hStatusBar, SB_SETTEXT, 2, (LPARAM)sstr.str().c_str());
    sstr.str("");

    SendMessage(_hStatusBar, SB_SETTEXT, 0, (LPARAM)"就绪");

    UpdateWindow(main_hwnd);
    Scanning = false;
    return 0;
} 

int BeginScan() {
    if(Scanning) return 1;
    if(!notFirstScan) {
        SendMessage(_hStatusBar, SB_SETTEXT, 0, (LPARAM)"扫描中");
        scanthrd = new std::thread(ScanThread);
        notFirstScan = true;
    } else {
        if(scanthrd->joinable()) {
            SendMessage(_hStatusBar, SB_SETTEXT, 0, (LPARAM)"挂起");
            scanthrd->join();
        }
        HANDLE updevent = CreateEventA(NULL, FALSE, FALSE, NULL);
        SendMessage(main_hwnd, WM_RESET, 0, (LPARAM)updevent);
        DWORD dwWaitResult = WaitForSingleObject(updevent, INFINITE);
        CloseHandle(updevent);

        windowcount = filteredwindowcount = 0;

        SendMessage(_hStatusBar, SB_SETTEXT, 0, (LPARAM)"扫描中");
        scanthrd = new std::thread(ScanThread);
    }
    return 0;
}

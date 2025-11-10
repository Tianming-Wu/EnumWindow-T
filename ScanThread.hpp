#pragma once

#include <string>
#include <windows.h>
#include <commctrl.h>

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

#define WM_ADDROOT WM_USER+1
#define WM_ADDCHILD WM_USER+2
#define WM_RESET WM_USER+3


int BeginScan();
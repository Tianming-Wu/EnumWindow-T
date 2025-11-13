#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <bitset>

//#include <dwmapi.h>
#include <psapi.h>

#include "toolset.h"
#include "config.h"
#include "PropertyWindowIDs.h"
#include "WindowOperations.hpp"

extern HINSTANCE g_hInstance;
//extern HANDLE g_quitevent;
extern bool gb_quitEvent;
extern HFONT hPublicFont;

#define WM_DATAPACKC WM_USER+12
#define WM_UPDATEPROPERTY WM_USER+13
#define WM_UPDATEEDITSTATE WM_USER+14
#define PW_TIMER_ID 2

struct MsgDataPackC {
    HWND hwnd;
    std::string targetClass, targetTitle;
    MsgDataPackC(std::string targetClass, std::string targetTitle, HWND hwnd)
        :hwnd(hwnd),targetClass(targetClass),targetTitle(targetTitle)
    {}
};

struct WindowInfo {
    HWND hwnd;
    int lx, ly, lw, lh, x, y, w, h;
    std::string Class, Title;
    struct{
        int x, y, w, h;
        std::string Title;
        bool cbSysMenu, cbVisible, cbMinBox, cbMaxBox, cbSizeBox, cbTopMost;
    } old;
    LONG windowLong, windowExLong;
    DWORD pid;
};

// void InitPropertyWindow() {
//     //quitEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
//     // property_Font = CreateFont(19, 0, 0, 0,
//     //     FW_MEDIUM, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
//     //     "Segoe UI" //"Consolas"
//     // );
// }

LRESULT CALLBACK PropertyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    thread_local static WindowInfo targetWindow;
    thread_local static BOOL editFlag = false; // 修改需要打开开关
    thread_local static BOOL flag_AgainstChange = true; // 修改检查屏蔽
    thread_local static UINT_PTR timer_id = 0;
    // Local controls
    enum {
        pw_hwndEditHwnd, pw_classEditHwnd, pw_titleEditHwnd, pw_posEditXHwnd, pw_posEditYHwnd, pw_posEditWHwnd, pw_posEditHHwnd,
        pw_styleEditHwnd, pw_styleExEditHwnd, pw_psPathEditHwnd, pw_psImageEditHwnd,
        pw_cbVisible, pw_cbSysMenu, pw_cbMinimizeBox, pw_cbMaximizeBox, pw_cbSizeBox, pw_cbTopMost,
        pw_viewPsPathButton,
        pw_wndRestoreButton, pw_wndApplyButton, pw_wndExitButton,
        pw_hStatusBar,
        pw_cbWndEdit,
        PWH_HwndID_MAXCOUNT
    };
    thread_local static HWND ctrls[PWH_HwndID_MAXCOUNT];

    //if(WaitForSingleObject(g_quitevent, 0) == WAIT_OBJECT_0) DestroyWindow(hwnd);
    if(gb_quitEvent) DestroyWindow(hwnd);
    switch(uMsg) {
    case WM_CREATE:
    {
        //SendMessage(hwnd, WM_SETFONT, WPARAM(property_Font), TRUE);
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance; //Window is 400*600

        ctrls[pw_hwndEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT,
            55, 5, 90, 24, hwnd, (HMENU)PWH_IDS_EDIT_HWND, hInstance, NULL);
        ctrls[pw_classEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 35, 255, 24, hwnd, (HMENU)PWH_IDS_EDIT_CLASS, hInstance, NULL);
        ctrls[pw_titleEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT|ES_AUTOHSCROLL,
            55, 65, 255, 24, hwnd, (HMENU)PWH_IDS_EDIT_TITLE, hInstance, NULL);
        ctrls[pw_posEditXHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT|ES_NUMBER|ES_AUTOHSCROLL,
            55, 95, 91, 24, hwnd, (HMENU)PWH_IDS_EDIT_POSX, hInstance, NULL);
        ctrls[pw_posEditYHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT|ES_NUMBER|ES_AUTOHSCROLL,
            55, 125, 91, 24, hwnd, (HMENU)PWH_IDS_EDIT_POSY, hInstance, NULL);
        ctrls[pw_posEditWHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT|ES_NUMBER|ES_AUTOHSCROLL,
            215, 95, 91, 24, hwnd, (HMENU)PWH_IDS_EDIT_SIZEW, hInstance, NULL);
        ctrls[pw_posEditHHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT|ES_NUMBER|ES_AUTOHSCROLL,
            215, 125, 91, 24, hwnd, (HMENU)PWH_IDS_EDIT_SIZEH, hInstance, NULL);
        
        ctrls[pw_styleEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 165, 270, 24, hwnd, (HMENU)PWH_IDS_EDIT_STYLE, hInstance, NULL);
        ctrls[pw_styleExEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 195, 270, 24, hwnd, (HMENU)PWH_IDS_EDIT_EXSTYLE, hInstance, NULL);

        ctrls[pw_psPathEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 225, 460, 24, hwnd, (HMENU)PWH_IDS_EDIT_PSPATH, hInstance, NULL);
        ctrls[pw_psImageEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 255, 460, 24, hwnd, (HMENU)PWH_IDS_EDIT_PSIMAGE, hInstance, NULL);

        ctrls[pw_viewPsPathButton] = CreateWindowEx(0, TEXT("Button"), TEXT("..."), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_FLAT,
            520, 224, 26, 26, hwnd, (HMENU)PWH_IDS_BTN_VIEWPSPATH, hInstance, NULL);
        

        ctrls[pw_cbVisible] = CreateWindowEx(0, TEXT("Button"), TEXT("VISIBLE"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 30, 80, 20, hwnd, (HMENU)PWH_CB_VISIBLE, hInstance, NULL);
        ctrls[pw_cbSysMenu] = CreateWindowEx(0, TEXT("Button"), TEXT("SYSMENU"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 52, 80, 20, hwnd, (HMENU)PWH_CB_SYSMENU, hInstance, NULL);
        ctrls[pw_cbMinimizeBox] = CreateWindowEx(0, TEXT("Button"), TEXT("MINIMIZEBOX"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 74, 110, 20, hwnd, (HMENU)PWH_CB_MINBOX, hInstance, NULL);
        ctrls[pw_cbMaximizeBox] = CreateWindowEx(0, TEXT("Button"), TEXT("MAXIMIZEBOX"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 96, 110, 20, hwnd, (HMENU)PWH_CB_MAXBOX, hInstance, NULL);
        ctrls[pw_cbSizeBox] = CreateWindowEx(0, TEXT("Button"), TEXT("SIZEBOX"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 117, 80, 20, hwnd, (HMENU)PWH_CB_SIZEBOX, hInstance, NULL);
        ctrls[pw_cbTopMost] = CreateWindowEx(0, TEXT("Button"), TEXT("TOPMOST"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            320, 140, 80, 20, hwnd, (HMENU)PWH_CB_TOPMOST, hInstance, NULL);

        ctrls[pw_wndRestoreButton] = CreateWindowEx(0, TEXT("Button"), TEXT("恢复"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_FLAT,
            290, 305, 90, 25, hwnd, (HMENU)PWH_BTN_RESTORE, hInstance, NULL);
        ctrls[pw_wndApplyButton] = CreateWindowEx(0, TEXT("Button"), TEXT("应用更改"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_FLAT,
            390, 305, 90, 25, hwnd, (HMENU)PWH_BTN_APPLY, hInstance, NULL);
        ctrls[pw_wndExitButton] = CreateWindowEx(0, TEXT("Button"), TEXT("取消"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_FLAT,
            490, 305, 90, 25, hwnd, (HMENU)PWH_BTN_CLOSE, hInstance, NULL);

        ctrls[pw_cbWndEdit] = CreateWindowEx(0, TEXT("Button"), TEXT("启用编辑"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            55, 305, 90, 25, hwnd, (HMENU)PWH_CB_EDITSWITCH, hInstance, NULL);

        ctrls[pw_hStatusBar] = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0,hwnd, (HMENU)PWH_IDS_STATUSBAR, hInstance, NULL);
        
        int StatusSizes[3] = { 70, 90, 65 };
        MakeSizeGrip(StatusSizes, 3);
        SendMessage(ctrls[pw_hStatusBar], SB_SETPARTS, 3, (LPARAM)StatusSizes);

        for(HWND &i : ctrls) SendMessage(i, WM_SETFONT, WPARAM(hPublicFont), TRUE);

        if(Config.PropertyWindow.AutoUpdateEnabled) {
            timer_id = SetTimer(hwnd, PW_TIMER_ID, Config.PropertyWindow.AutoUpdateInterval, NULL);
            SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)(std::to_string(Config.PropertyWindow.AutoUpdateInterval)+"ms").c_str());
        } else SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)"禁用");

        //SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)(std::to_string(timer_id)).c_str());

        editFlag = false;
        SendMessage(hwnd, WM_UPDATEEDITSTATE, 0, 0);
        SendMessageA(ctrls[pw_cbWndEdit], BM_SETCHECK, BST_UNCHECKED, 0);

        flag_AgainstChange = true; // 启用阻止文本框状态更新
        break;
    }
    case WM_TIMER: // 自动更新计时器
        if(wParam != PW_TIMER_ID) break;
    case WM_UPDATEPROPERTY:
    {
        flag_AgainstChange = true;
        if(editFlag) break;
        std::stringstream sstr; std::string tstr;

        char windowTitle[256];
        GetWindowText(targetWindow.hwnd, windowTitle, sizeof(windowTitle));
        if(std::string(windowTitle) != targetWindow.Title) // Window Title has been changed.
        {
            targetWindow.Title = windowTitle;
            SetWindowText(ctrls[pw_titleEditHwnd], targetWindow.Title.c_str());
        }
        RECT tWndRect; int pts[4];
        GetWindowRect(targetWindow.hwnd, &tWndRect);
        pts[0] = targetWindow.x = tWndRect.left;
        pts[1] = targetWindow.y = tWndRect.top;
        pts[2] = targetWindow.w = tWndRect.right-tWndRect.left;
        pts[3] = targetWindow.h = tWndRect.bottom-tWndRect.top;

        for(int p=0,i=pw_posEditXHwnd; i<=pw_posEditHHwnd; i++,p++) {
            int textLength = GetWindowTextLength(targetWindow.hwnd);
            std::string tg = std::to_string(pts[p]);
            char* pw = new char[textLength];
            GetWindowText(ctrls[i], (LPSTR)pw, textLength);
            if(std::string(pw) != tg) SetWindowText(ctrls[i], tg.c_str());
            delete pw;
        }

        LONG temp_long;
        if(targetWindow.windowLong != ( temp_long = GetWindowLong(targetWindow.hwnd, GWL_STYLE)))
        {
            std::bitset<sizeof(LONG)*8> bs;
            targetWindow.windowLong = temp_long;
            SetWindowText(ctrls[pw_styleEditHwnd], (bs=targetWindow.windowLong).to_string().c_str());
        }

        if(targetWindow.windowExLong != ( temp_long = GetWindowLong(targetWindow.hwnd, GWL_EXSTYLE)))
        {
            std::bitset<sizeof(LONG)*8> bs;
            targetWindow.windowExLong = temp_long;
            SetWindowText(ctrls[pw_styleExEditHwnd], (bs=targetWindow.windowExLong).to_string().c_str());
        }

        bool cbSysMenu = (targetWindow.windowLong & WS_SYSMENU) != 0;
        if(cbSysMenu) SendMessage(ctrls[pw_cbSysMenu], BM_SETCHECK, BST_CHECKED, 0);
        bool cbVisible = (targetWindow.windowLong & WS_VISIBLE) != 0;
        if(cbVisible) SendMessage(ctrls[pw_cbVisible], BM_SETCHECK, BST_CHECKED, 0);
        bool cbMinBox = (targetWindow.windowLong & WS_MINIMIZEBOX) != 0;
        if(cbMinBox) SendMessage(ctrls[pw_cbMinimizeBox], BM_SETCHECK, BST_CHECKED, 0);
        bool cbMaxBox = (targetWindow.windowLong & WS_MAXIMIZEBOX) != 0;
        if(cbMaxBox) SendMessage(ctrls[pw_cbMaximizeBox], BM_SETCHECK, BST_CHECKED, 0);
        bool cbSizeBox = (targetWindow.windowLong & WS_SIZEBOX) != 0;
        if(cbSizeBox) SendMessage(ctrls[pw_cbSizeBox], BM_SETCHECK, BST_CHECKED, 0);

        bool cbTopMost = (targetWindow.windowExLong & WS_EX_TOPMOST) != 0;
        if(cbTopMost) SendMessage(ctrls[pw_cbTopMost], BM_SETCHECK, BST_CHECKED, 0);

        flag_AgainstChange = false;
        break;
    }
    case WM_UPDATEEDITSTATE: // 编辑状态修改信息
    {
        // Title
        SendMessage(ctrls[pw_titleEditHwnd], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        //Position
        SendMessage(ctrls[pw_posEditXHwnd], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_posEditYHwnd], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_posEditWHwnd], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_posEditHHwnd], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        // Properties
        SendMessage(ctrls[pw_cbVisible], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_cbSysMenu], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_cbMinimizeBox], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_cbMaximizeBox], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_cbSizeBox], EM_SETREADONLY, (LPARAM)!editFlag, 0);
        SendMessage(ctrls[pw_cbTopMost], EM_SETREADONLY, (LPARAM)!editFlag, 0);

        break;
    }
    case WM_DATAPACKC: // 接受线程发送的数据包
    {
        flag_AgainstChange = true;
        MsgDataPackC* data = reinterpret_cast<MsgDataPackC*>(lParam);
        targetWindow.Class = data->targetClass,
        targetWindow.Title = targetWindow.old.Title = data->targetTitle,
        targetWindow.hwnd = data->hwnd;

        std::stringstream tsstr; std::string tstr;

        SetWindowText(ctrls[pw_classEditHwnd], targetWindow.Class.c_str()); // 显示窗口类
        SetWindowText(ctrls[pw_titleEditHwnd], targetWindow.Title.c_str()); // 显示窗口标题

        tsstr << std::hex << targetWindow.hwnd; tstr = tsstr.str(); tsstr.str("");
        tstr = tstr.substr(tstr.length()-6); tsstr << "0x" << tstr;
        SetWindowText(ctrls[pw_hwndEditHwnd], tsstr.str().c_str()); // 显示窗口句柄（16进制）
        SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 1, (LPARAM)tsstr.str().c_str());
        tsstr.str("");

        // NOTE: 线程内部逻辑更新顺序有误，此处后续将移至初始化部分。
        RECT tWndRect; int pts[4];
        GetWindowRect(targetWindow.hwnd, &tWndRect);
        pts[0] = targetWindow.lx=targetWindow.x = tWndRect.left;
        pts[1] = targetWindow.ly=targetWindow.y = tWndRect.top;
        pts[2] = targetWindow.lw=targetWindow.w = tWndRect.right-tWndRect.left;
        pts[3] = targetWindow.lh=targetWindow.h = tWndRect.bottom-tWndRect.top;

        SetWindowText(ctrls[pw_posEditXHwnd], std::to_string(pts[0]).c_str());
        SetWindowText(ctrls[pw_posEditYHwnd], std::to_string(pts[1]).c_str());
        SetWindowText(ctrls[pw_posEditWHwnd], std::to_string(pts[2]).c_str());
        SetWindowText(ctrls[pw_posEditHHwnd], std::to_string(pts[3]).c_str());

        GetWindowThreadProcessId(targetWindow.hwnd, &targetWindow.pid); // Get Window Process information.
        SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 0, (LPARAM)std::to_string(targetWindow.pid).c_str());
        std::string psPath, psImage;
        psPath.resize(Config.MaxPathLength); psImage.resize(Config.MaxPathLength);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetWindow.pid);
        auto len = GetModuleFileNameEx(hProcess, NULL, (LPSTR)psPath.data(), (DWORD)Config.MaxPathLength);
        GetProcessImageFileName(hProcess, (LPSTR)psImage.data(), (DWORD)Config.MaxPathLength);

        SetWindowText(ctrls[pw_psPathEditHwnd], psPath.c_str());
        SetWindowText(ctrls[pw_psImageEditHwnd], psImage.c_str());
        
        LONG windowLong = GetWindowLong(targetWindow.hwnd, GWL_STYLE); // Get Window information.
        LONG windowExLong = GetWindowLong(targetWindow.hwnd, GWL_EXSTYLE);

        targetWindow.old.cbSysMenu = (windowLong & WS_SYSMENU) != 0;
        targetWindow.old.cbVisible = (windowLong & WS_VISIBLE) != 0;
        targetWindow.old.cbMinBox = (windowLong & WS_MINIMIZEBOX) != 0;
        targetWindow.old.cbMaxBox = (windowLong & WS_MAXIMIZEBOX) != 0;
        targetWindow.old.cbSizeBox = (windowLong & WS_SIZEBOX) != 0;

        targetWindow.old.cbTopMost = (windowExLong & WS_EX_TOPMOST) != 0;

        flag_AgainstChange = false;
        SendMessage(hwnd, WM_UPDATEPROPERTY, 0, 0);
        break;
    }
    case WM_PW_CONFIGUPD: // 配置更新
    {
        if(!editFlag) {
            if(timer_id!=0) KillTimer(hwnd, timer_id);
            if(Config.PropertyWindow.AutoUpdateEnabled) {
                timer_id = SetTimer(hwnd, PW_TIMER_ID, Config.PropertyWindow.AutoUpdateInterval, NULL);
                SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)(std::to_string(Config.PropertyWindow.AutoUpdateInterval)+"ms").c_str());
            } else
                SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)"禁用");
        }
        
        break;
    }
    case WM_COMMAND:
    {
        switch(HIWORD(wParam)) {
        case BN_CLICKED: // 按钮被按下
        {
            switch(LOWORD(wParam)) {
            case PWH_BTN_RESTORE: // 恢复创建 PropertyWindow 时的属性
            {
                RECT resWndRect;
                GetWindowRect(targetWindow.hwnd, &resWndRect);
                int rx = resWndRect.left, ry = resWndRect.top, rw = resWndRect.right-resWndRect.left, rh = resWndRect.bottom-resWndRect.top;
                if( (rx!=targetWindow.lx) || (ry!=targetWindow.ly) || (rw!=targetWindow.lw) || (rh!=targetWindow.lh) ) {
                    MoveWindow(targetWindow.hwnd, targetWindow.lx, targetWindow.ly, targetWindow.lw, targetWindow.lh, TRUE);
                }
                
                SetWindowText(targetWindow.hwnd, (LPSTR)targetWindow.old.Title.data());

                LONG windowLong = GetWindowLong(targetWindow.hwnd, GWL_STYLE);
                LONG windowExLong = GetWindowLong(targetWindow.hwnd, GWL_EXSTYLE);

                targetWindow.old.cbVisible?(windowLong|=WS_VISIBLE):(windowLong&=~WS_VISIBLE);
                targetWindow.old.cbSysMenu?(windowLong|=WS_SYSMENU):(windowLong&=~WS_SYSMENU);
                targetWindow.old.cbMinBox ?(windowLong|=WS_MINIMIZEBOX):(windowLong&=~WS_MINIMIZEBOX);
                targetWindow.old.cbMaxBox ?(windowLong|=WS_MAXIMIZEBOX):(windowLong&=~WS_MAXIMIZEBOX);
                targetWindow.old.cbSizeBox?(windowLong|=WS_SIZEBOX):(windowLong&=~WS_SIZEBOX);

                bool cbTopMostBak = (windowExLong & WS_EX_TOPMOST) != 0;
                targetWindow.old.cbTopMost?(windowExLong!=WS_EX_TOPMOST):(windowExLong&=~WS_EX_TOPMOST);

                SetWindowLong(targetWindow.hwnd, GWL_STYLE, windowLong);
                SetWindowLong(targetWindow.hwnd, GWL_EXSTYLE, windowExLong);

                if(targetWindow.old.cbTopMost != cbTopMostBak) SetWindowPos(targetWindow.hwnd, (targetWindow.old.cbTopMost)?(HWND_TOPMOST):(HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

                // 取消修改开关
                editFlag = false;
                SendMessage(hwnd, WM_UPDATEEDITSTATE, 0, 0);
                SendMessageA(ctrls[pw_cbWndEdit], BM_SETCHECK, BST_UNCHECKED, 0);

                if(Config.PropertyWindow.AutoUpdateEnabled) {
                    timer_id = SetTimer(hwnd, PW_TIMER_ID, Config.PropertyWindow.AutoUpdateInterval, NULL);
                    SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)(std::to_string(Config.PropertyWindow.AutoUpdateInterval)+"ms").c_str());
                }
                SendMessage(hwnd, WM_UPDATEPROPERTY, 0, 0);
                break;
            }
            case PWH_BTN_APPLY: // 应用对窗口的修改
            {
                int Apply_Error_Level;

                std::string postr[4];
                for(auto &i: postr) i.resize(16);
                GetWindowText(ctrls[pw_posEditXHwnd], (LPSTR)postr[0].data(), 16);
                GetWindowText(ctrls[pw_posEditYHwnd], (LPSTR)postr[1].data(), 16);
                GetWindowText(ctrls[pw_posEditWHwnd], (LPSTR)postr[2].data(), 16);
                GetWindowText(ctrls[pw_posEditHHwnd], (LPSTR)postr[3].data(), 16);
                int nx =  stoi(postr[0]), ny = stoi(postr[1]), nw = stoi(postr[2]), nh = stoi(postr[3]); // Target Size
                if( (nx!=targetWindow.x) || (ny!=targetWindow.y) || (nw!=targetWindow.w) || (nh!=targetWindow.h) ) {
                    MoveWindow(targetWindow.hwnd, nx, ny, nw, nh, TRUE);
                    RECT resWndRect;
                    GetWindowRect(targetWindow.hwnd, &resWndRect);
                    int rx = resWndRect.left, ry = resWndRect.top, rw = resWndRect.right-resWndRect.left, rh = resWndRect.bottom-resWndRect.top; // Actual Size
                    if( (rx!=nx) || (ry!=ny) || (rw!=nw) || (rh!=nh) ) Apply_Error_Level |= APPLYERR_RESIZEFAILED; // Check if operation succeeded
                }
                
                int titleSize = GetWindowTextLength(ctrls[pw_titleEditHwnd]);
                std::string newTitle; newTitle.resize(titleSize+1);
                GetWindowText(ctrls[pw_titleEditHwnd], (LPSTR)newTitle.data(), titleSize+1);
                SetWindowText(targetWindow.hwnd, (LPSTR)newTitle.data());

                // 获取目标窗口的属性
                LONG windowLong = GetWindowLong(targetWindow.hwnd, GWL_STYLE);
                LONG windowExLong = GetWindowLong(targetWindow.hwnd, GWL_EXSTYLE);

                // 从UI中获得更改信息
                bool cbSysMenu, cbVisible, cbMinBox, cbMaxBox, cbSizeBox, cbTopMost, cbTopMostBak;
                cbVisible = (SendMessage(ctrls[pw_cbVisible], BM_GETCHECK, 0, 0) == BST_CHECKED);
                cbSysMenu = (SendMessage(ctrls[pw_cbSysMenu], BM_GETCHECK, 0, 0) == BST_CHECKED);
                cbMinBox  = (SendMessage(ctrls[pw_cbMinimizeBox], BM_GETCHECK, 0, 0) == BST_CHECKED);
                cbMaxBox  = (SendMessage(ctrls[pw_cbMaximizeBox], BM_GETCHECK, 0, 0) == BST_CHECKED);
                cbSizeBox = (SendMessage(ctrls[pw_cbSizeBox], BM_GETCHECK, 0, 0) == BST_CHECKED);
                cbTopMost = (SendMessage(ctrls[pw_cbTopMost], BM_GETCHECK, 0, 0) == BST_CHECKED);

                // 将更改写入变量
                cbVisible?(windowLong|=WS_VISIBLE):(windowLong&=~WS_VISIBLE);
                cbSysMenu?(windowLong|=WS_SYSMENU):(windowLong&=~WS_SYSMENU);
                cbMinBox ?(windowLong|=WS_MINIMIZEBOX):(windowLong&=~WS_MINIMIZEBOX);
                cbMaxBox ?(windowLong|=WS_MAXIMIZEBOX):(windowLong&=~WS_MAXIMIZEBOX);
                cbSizeBox?(windowLong|=WS_SIZEBOX):(windowLong&=~WS_SIZEBOX);

                cbTopMostBak = (windowExLong & WS_EX_TOPMOST) != 0;
                cbTopMost?(windowExLong!=WS_EX_TOPMOST):(windowExLong&=~WS_EX_TOPMOST);

                // 应用更改到窗口
                SetWindowLong(targetWindow.hwnd, GWL_STYLE, windowLong);
                SetWindowLong(targetWindow.hwnd, GWL_EXSTYLE, windowExLong);

                // 额外处理置顶状态更改
                if(cbTopMost != cbTopMostBak) SetWindowPos(targetWindow.hwnd, (cbTopMost)?(HWND_TOPMOST):(HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

                // 取消修改开关
                editFlag = false;
                SendMessage(hwnd, WM_UPDATEEDITSTATE, 0, 0);
                SendMessageA(ctrls[pw_cbWndEdit], BM_SETCHECK, BST_UNCHECKED, 0);

                // 启动自动刷新
                if(Config.PropertyWindow.AutoUpdateEnabled) {
                    timer_id = SetTimer(hwnd, PW_TIMER_ID, Config.PropertyWindow.AutoUpdateInterval, NULL);
                    SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)(std::to_string(Config.PropertyWindow.AutoUpdateInterval)+"ms").c_str());
                }
                break;
            }
            case PWH_CB_EDITSWITCH: // 编辑开关切换
            {
                editFlag = (SendMessage(ctrls[pw_cbWndEdit], BM_GETCHECK, 0, 0) == BST_CHECKED);
                SendMessage(hwnd, WM_UPDATEEDITSTATE, 0, 0);
                break;
            }
            case PWH_BTN_CLOSE: // 关闭窗口
            {
                DestroyWindow(hwnd);
                break;
            }
            case PWH_IDS_BTN_VIEWPSPATH: // 点击“打开路径”按钮
            {
                STARTUPINFOA si = { 0 };
                si.cb = sizeof(si);

                int pathSize = GetWindowTextLength(ctrls[pw_psPathEditHwnd]);
                std::string path; path.resize(pathSize+1);
                GetWindowText(ctrls[pw_psPathEditHwnd], (LPSTR)path.data(), pathSize+1);
                SetWindowText(targetWindow.hwnd, (LPSTR)path.data());

                // std::string cmdline = "explorer /select,\"" + path + "\"";
                HINSTANCE result = ShellExecuteA(
                    NULL,
                    "open",
                    "explorer.exe",
                    ("/select,\"" + path + "\"").c_str(),
                    NULL,
                    SW_SHOWNORMAL
                );

                // MessageBox(hwnd, (LPSTR)cmdline.c_str(), "DEBUG", MB_OK);

                if(!result) {
                    std::string errmsg = "Failed to start explorer.";
                    MessageBox(hwnd, (LPSTR)errmsg.c_str(), "ERROR", MB_ICONHAND|MB_OK);
                }
                break;
            }
            case PWH_CB_VISIBLE: // 属性复选框修改
            case PWH_CB_SYSMENU:
            case PWH_CB_MINBOX:
            case PWH_CB_MAXBOX:
            case PWH_CB_SIZEBOX:
            case PWH_CB_TOPMOST:
            {
                if(!flag_AgainstChange) // 是用户进行的修改
                {
                    if(Config.PropertyWindow.AutoUpdateEnabled) { // 停止自动更新计时器
                        KillTimer(hwnd, timer_id);
                        SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)"暂停");
                    }
                }
                break;
            }
            }
            break;
        }
        case EN_CHANGE: // 编辑框被修改
        {
            if(!flag_AgainstChange) // 是用户进行的修改
            {
                if(Config.PropertyWindow.AutoUpdateEnabled) { // 停止自动更新计时器
                    KillTimer(hwnd, timer_id);
                    SendMessage(ctrls[pw_hStatusBar], SB_SETTEXT, 2, (LPARAM)"暂停");
                }
            }
            break;
        }
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT wndRect, sizeRect;
        GetClientRect(hwnd, &wndRect);
        sizeRect.bottom = wndRect.bottom - wndRect.top; sizeRect.right = wndRect.right-wndRect.left;

        std::stringstream sstr;
        
        SelectObject(hdc, hPublicFont);
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, TRANSPARENT);

        DrawTextStd(hdc, "hwnd", &posRect(15, 7), DTS_DEFAULT);
        DrawTextStd(hdc, "类名", &posRect(15, 37), DTS_DEFAULT);
        DrawTextStd(hdc, "标题", &posRect(15, 67), DTS_DEFAULT);
        DrawTextStd(hdc, "Xpos", &posRect(15, 97), DTS_DEFAULT);
        DrawTextStd(hdc, "Ypos", &posRect(15, 127), DTS_DEFAULT);
        DrawTextStd(hdc, "Width", &posRect(170, 97), DTS_DEFAULT);
        DrawTextStd(hdc, "Height", &posRect(170, 127), DTS_DEFAULT);
        DrawTextStd(hdc, "样式", &posRect(15, 167), DTS_DEFAULT);
        DrawTextStd(hdc, "EX样式", &posRect(15, 197), DTS_DEFAULT);
        DrawTextStd(hdc, "路径", &posRect(15, 227), DTS_DEFAULT);
        DrawTextStd(hdc, "映像", &posRect(15, 257), DTS_DEFAULT);

        sstr << "原尺寸：(" << targetWindow.lx << ", " << targetWindow.ly << "), "
             << targetWindow.lw << "*" << targetWindow.lh;
        DrawTextStd(hdc, sstr.str(), &posRect(155, 7), DTS_DEFAULT);
        sstr.str("");

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        if(Config.PropertyWindow.AutoUpdateEnabled) KillTimer(hwnd, timer_id);
        PostQuitMessage(0);
        //SendMessage(hwnd, 0, 0, 0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int PropertyWindow_WorkerThread(HWND hwnd) {
    HWND propertyWindow;
    
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = PropertyWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_PropertyWindow{}");

    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassExA(&wcex);

    char className[256], windowTitle[256];
    GetClassName(hwnd, className, sizeof(className));
    GetWindowText(hwnd, windowTitle, sizeof(windowTitle));

    std::stringstream sstr;
    sstr << "属性: " << className;
    if(!std::string(windowTitle).empty()) sstr << " | " << windowTitle;
    sstr << "(" << std::hex << hwnd << ")";

    propertyWindow = CreateWindowA(
        TEXT("WindowContolExClass_PropertyWindow{}"), sstr.str().c_str(),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 610, 400,
        NULL, NULL, g_hInstance, NULL
    );

    subWindows.push_back(subWindowItem(propertyWindow, ST_Property));

    ShowWindow(propertyWindow, SW_SHOW);

    MsgDataPackC data(className, windowTitle, hwnd);
    PostMessage(propertyWindow, WM_DATAPACKC, 0, reinterpret_cast<LPARAM>(&data));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //MessageBox(NULL, "PropertyWindow Thread OK", "Info", MB_OK);

    return (int)msg.wParam;
}


int StartPropertyWindow(HWND hwnd) {
    std::thread* thrd = new std::thread(PropertyWindow_WorkerThread, hwnd);
    subThreadItem item { thrd, ST_Property, nullptr };
    subThreads.push_back(item);
    return 0;
}


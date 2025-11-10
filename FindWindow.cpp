#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "FindWindowIDs.h"
#include "toolset.h"

#include "PropertyWindow.hpp"

extern HINSTANCE g_hInstance;
extern HWND hTreeView;
//extern HANDLE g_quitevent;
extern bool gb_quitEvent;
extern HFONT hPublicFont;

bool FindWindowRunning = false;
HWND s_findWindowHWND = nullptr;

void TrackMouseLeave(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
    tme.hwndTrack = hwnd;
    tme.dwHoverTime = HOVER_DEFAULT;

    TrackMouseEvent(&tme);
}

LRESULT CALLBACK FindWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    thread_local static HCURSOR hDragCursor, hNormalCursor;
    thread_local static BOOL g_bDrag = FALSE;
    thread_local static HWND g_hTargetWindow = nullptr, g_lTargWnd = nullptr;
    thread_local static POINT g_MousePos;
    //thread_local static HBRUSH hTransparentBrush;

    thread_local bool first_active = true;

    thread_local static const RECT dragRect = makeRectRelative(15,10,32,32);

    enum { // IDs as the auto index of the hwnd storage. awa
        fw_hwndEditHwnd, fw_classEditHwnd, fw_titleEditHwnd, fw_sizeEditHwnd, fw_processEditHwnd,
        fw_treeBtnHwnd, fw_propertyBtnHwnd,
        FWH_HwndID_MAXCOUNT
    };

    thread_local static HWND ctrls[FWH_HwndID_MAXCOUNT];

    //if(WaitForSingleObject(g_quitevent, 0) == WAIT_OBJECT_0) DestroyWindow(hwnd);
    if(gb_quitEvent) DestroyWindow(hwnd);
    switch(uMsg) {
    case WM_CREATE:
    {
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        ctrls[fw_hwndEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT,
            55, 55, 90, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[fw_classEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 85, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[fw_titleEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 115, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[fw_sizeEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 145, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);

        ctrls[fw_treeBtnHwnd] = CreateWindowEx(0, TEXT("Button"), TEXT("查看列表"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            185, 230, 90, 25, hwnd, (HMENU)FWH_ID_TREEBTN, hInstance, NULL);
        ctrls[fw_propertyBtnHwnd] = CreateWindowEx(0, TEXT("Button"), TEXT("查看属性"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            285, 230, 90, 25, hwnd, (HMENU)FWH_ID_PROPERTYBTN, hInstance, NULL);

        hDragCursor = LoadCursor(NULL, IDC_CROSS);
        hNormalCursor = LoadCursor(NULL, IDC_ARROW);

        //hTransparentBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

        for(HWND &i: ctrls) SendMessage(i, WM_SETFONT, WPARAM(hPublicFont), TRUE);

        break;
    }
    case WM_LBUTTONDOWN:
    {
        POINT mousePos;
        mousePos.x = LOWORD(lParam);
        mousePos.y = HIWORD(lParam);

        if (PtInRect(&dragRect, mousePos))
            g_bDrag = true;

        //g_bDrag = DragDetect(hwnd, g_MousePos);
        if (g_bDrag)
            {
                SetCursor(hDragCursor);
                SetCapture(hwnd); // 捕获鼠标消息
                TrackMouseLeave(hwnd); // 开始监测鼠标离开事件
                g_hTargetWindow = NULL;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        break;
    }
    case WM_MOUSEMOVE:
        if (g_bDrag)
        {
            // 获取鼠标当前位置
            GetCursorPos(&g_MousePos);

            // 获取鼠标下方的窗口句柄
            g_hTargetWindow = WindowFromPoint(g_MousePos);

            if(g_hTargetWindow != g_lTargWnd) {
                if(first_active) {
                    EnableWindow(ctrls[fw_treeBtnHwnd], TRUE);
                    EnableWindow(ctrls[fw_propertyBtnHwnd], TRUE);
                    first_active = false;
                }

                std::stringstream sstr; std::string tstr;
                sstr << std::hex << g_hTargetWindow; tstr = sstr.str(); sstr.str("");
                SetWindowText(hwnd, (LPSTR)(std::string("查找窗口 - ")+tstr).c_str());
                tstr = tstr.substr(tstr.length()-6); sstr << "0x" << tstr;
                SetWindowText(ctrls[fw_hwndEditHwnd], sstr.str().c_str());
                sstr.str("");

                char windowClass[256], windowTitle[256];
                GetClassName(g_hTargetWindow, windowClass, sizeof(windowClass));
                GetWindowText(g_hTargetWindow, windowTitle, sizeof(windowTitle));
                SetWindowText(ctrls[fw_classEditHwnd], windowClass);
                SetWindowText(ctrls[fw_titleEditHwnd], windowTitle);

                RECT tR;  std::stringstream sstr1;
                GetWindowRect(g_hTargetWindow, &tR);
                sstr1 << "(" << tR.left << ", " << tR.top << "), " << tR.right-tR.left << "*" << tR.bottom-tR.top;
                SetWindowText(ctrls[fw_sizeEditHwnd], sstr1.str().c_str());
                sstr1.str("");

                g_lTargWnd = g_hTargetWindow;
            }
        }
        break;
    case WM_LBUTTONUP:
        if (g_bDrag)
        {
            SetCursor(hNormalCursor);
            ReleaseCapture(); // 释放鼠标捕获
            g_bDrag = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_MOUSELEAVE:
        if (g_bDrag) g_hTargetWindow = g_lTargWnd = NULL;
        break;
    case WM_COMMAND:
    {
        switch(HIWORD(wParam)){
        case BN_CLICKED: // 按下按钮
            switch(LOWORD(wParam))
            {
            case FWH_ID_TREEBTN:
            {
                HTREEITEM item = TreeView_MapAccIDToHTREEITEM(hTreeView, (WPARAM)g_hTargetWindow);
                if(item) TreeView_SelectItem(hTreeView, item);
                    else MessageBox(NULL, "Failed", "ERROR", 0);
                break;
            }
            case FWH_ID_PROPERTYBTN:
            {
                StartPropertyWindow(g_hTargetWindow);
                break;
            }
            }
            break;
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

        std::stringstream sstr; std::string tstr;
        
        SelectObject(hdc, hPublicFont);
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, TRANSPARENT);

        DrawTextStd(hdc, "拖动光标查找窗口", &posRect(62, 14), DTS_DEFAULT);
        DrawTextStd(hdc, "hwnd", &posRect(15, 57), DTS_DEFAULT);
        DrawTextStd(hdc, "类名", &posRect(15, 87), DTS_DEFAULT);
        DrawTextStd(hdc, "标题", &posRect(15, 117), DTS_DEFAULT);
        DrawTextStd(hdc, "尺寸", &posRect(15, 147), DTS_DEFAULT);

        //HBRUSH hBrush = CreatePatternBrush(GetStockObject(HOLLOW_BRUSH));
        //HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hTransparentBrush);
        
        Rectangle(hdc, dragRect.left-1, dragRect.top-1, dragRect.right+1, dragRect.bottom+1);
        if(!g_bDrag) DrawIconEx(hdc, dragRect.left, dragRect.top, hDragCursor, 0, 0, 0, (HBRUSH)GetStockObject(COLOR_BACKGROUND) /*hTransparentBrush*/, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
            else FillRect(hdc, &dragRect, (HBRUSH)GetStockObject(COLOR_BACKGROUND));

        //SelectObject(hdc, hOldBrush);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int FindWindow_WorkerThread() {
    HWND findWindow;
    FindWindowRunning = true;
    
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = FindWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_FindWindow{}");
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassExA(&wcex);

    findWindow = CreateWindowEx(
        /*WS_EX_NOREDIRECTIONBITMAP,*/ 0,
        TEXT("WindowContolExClass_FindWindow{}"), TEXT("查找窗口"),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, g_hInstance, NULL
    );
    s_findWindowHWND = findWindow;

    subWindows.push_back(findWindow);

    ShowWindow(findWindow, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FindWindowRunning = false;
    s_findWindowHWND = nullptr;

    return (int)msg.wParam;
}


int StartFindWindow() {
    if(!FindWindowRunning) {
        std::thread* thrd = new std::thread(FindWindow_WorkerThread);
        subThreadItem item { thrd, ST_FindWindow, nullptr };
        subThreads.push_back(item);
    } else SetForegroundWindow(s_findWindowHWND);
    return 0;
}
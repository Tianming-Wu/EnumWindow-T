#pragma once

#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "TargetWindowIDs.h"
#include "toolset.h"

#include "PropertyWindow.hpp"

extern HINSTANCE g_hInstance;
extern HWND hTreeView;
//extern HANDLE g_quitevent;
extern bool gb_quitEvent;
extern HFONT hPublicFont;

bool TargetWindowRunning = false;
HWND s_targetWindowHWND = nullptr;

extern HWND main_hwnd;

void TrackMouseLeave(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
    tme.hwndTrack = hwnd;
    tme.dwHoverTime = HOVER_DEFAULT;

    TrackMouseEvent(&tme);
}

LRESULT CALLBACK TargetWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    thread_local static HCURSOR hDragCursor, hNormalCursor;
    thread_local static BOOL g_bDrag = FALSE;
    thread_local static HWND g_hTargetWindow = nullptr, g_lTargWnd = nullptr;
    thread_local static POINT g_MousePos;

    thread_local bool first_active = true;

    thread_local static const RECT dragRect = makeRectRelative(15,10,32,32);

    enum {
        tw_hwndEditHwnd, tw_classEditHwnd, tw_titleEditHwnd, tw_sizeEditHwnd, tw_processEditHwnd,
        tw_treeBtnHwnd, tw_propertyBtnHwnd,
        TWH_HwndID_MAXCOUNT
    };

    thread_local static HWND ctrls[TWH_HwndID_MAXCOUNT];

    if(gb_quitEvent) DestroyWindow(hwnd);
    switch(uMsg) {
    case WM_CREATE:
    {
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        ctrls[tw_hwndEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT,
            55, 55, 90, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[tw_classEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 85, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[tw_titleEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 115, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[tw_sizeEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_READONLY|ES_LEFT|ES_AUTOHSCROLL,
            55, 145, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);

        ctrls[tw_treeBtnHwnd] = CreateWindowEx(0, TEXT("Button"), TEXT("查看列表"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            185, 230, 90, 25, hwnd, (HMENU)TWH_ID_TREEBTN, hInstance, NULL);
        ctrls[tw_propertyBtnHwnd] = CreateWindowEx(0, TEXT("Button"), TEXT("查看属性"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            285, 230, 90, 25, hwnd, (HMENU)TWH_ID_PROPERTYBTN, hInstance, NULL);

        hDragCursor = LoadCursor(NULL, IDC_CROSS);
        hNormalCursor = LoadCursor(NULL, IDC_ARROW);

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

        if (g_bDrag)
            {
                SetCursor(hDragCursor);
                SetCapture(hwnd);
                TrackMouseLeave(hwnd);
                g_hTargetWindow = NULL;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        break;
    }
    case WM_MOUSEMOVE:
        if (g_bDrag)
        {
            GetCursorPos(&g_MousePos);
            g_hTargetWindow = WindowFromPoint(g_MousePos);

            if(g_hTargetWindow != g_lTargWnd) {
                if(first_active) {
                    EnableWindow(ctrls[tw_treeBtnHwnd], TRUE);
                    EnableWindow(ctrls[tw_propertyBtnHwnd], TRUE);
                    first_active = false;
                }

                std::stringstream sstr; std::string tstr;
                sstr << std::hex << g_hTargetWindow; tstr = sstr.str(); sstr.str("");
                SetWindowText(hwnd, (LPSTR)(std::string("目标窗口 - ")+tstr).c_str());
                tstr = tstr.substr(tstr.length()-6); sstr << "0x" << tstr;
                SetWindowText(ctrls[tw_hwndEditHwnd], sstr.str().c_str());
                sstr.str("");

                char windowClass[256], windowTitle[256];
                GetClassName(g_hTargetWindow, windowClass, sizeof(windowClass));
                GetWindowText(g_hTargetWindow, windowTitle, sizeof(windowTitle));
                SetWindowText(ctrls[tw_classEditHwnd], windowClass);
                SetWindowText(ctrls[tw_titleEditHwnd], windowTitle);

                RECT tR;  std::stringstream sstr1;
                GetWindowRect(g_hTargetWindow, &tR);
                sstr1 << "(" << tR.left << ", " << tR.top << "), " << tR.right-tR.left << "*" << tR.bottom-tR.top;
                SetWindowText(ctrls[tw_sizeEditHwnd], sstr1.str().c_str());
                sstr1.str("");

                g_lTargWnd = g_hTargetWindow;
            }
        }
        break;
    case WM_LBUTTONUP:
        if (g_bDrag)
        {
            SetCursor(hNormalCursor);
            ReleaseCapture();
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
        case BN_CLICKED:
            switch(LOWORD(wParam))
            {
            case TWH_ID_TREEBTN:
            {
                HTREEITEM item = FindTreeViewItemByHwnd(hTreeView, g_hTargetWindow);
                if(item) {
                    FocusControlAcrossThreads(hTreeView);
                    TreeView_SelectItem(hTreeView, item);
                } else MessageBox(NULL, "无法找到对应列表项。\n尝试刷新列表？", "错误", 0);
                break;
            }
            case TWH_ID_PROPERTYBTN:
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

        Rectangle(hdc, dragRect.left-1, dragRect.top-1, dragRect.right+1, dragRect.bottom+1);
        if(!g_bDrag) DrawIconEx(hdc, dragRect.left, dragRect.top, hDragCursor, 0, 0, 0, (HBRUSH)GetStockObject(COLOR_BACKGROUND), DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
            else FillRect(hdc, &dragRect, (HBRUSH)GetStockObject(COLOR_BACKGROUND));

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

int TargetWindow_WorkerThread() {
    HWND targetWindow;
    TargetWindowRunning = true;
    
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = TargetWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_TargetWindow{}");
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassExA(&wcex);

    targetWindow = CreateWindowEx(
        0,
        TEXT("WindowContolExClass_TargetWindow{}"), TEXT("目标窗口"),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, g_hInstance, NULL
    );
    s_targetWindowHWND = targetWindow;

    subWindows.push_back(targetWindow);

    ShowWindow(targetWindow, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TargetWindowRunning = false;
    s_targetWindowHWND = nullptr;

    return (int)msg.wParam;
}


int StartTargetWindow() {
    if(!TargetWindowRunning) {
        std::thread* thrd = new std::thread(TargetWindow_WorkerThread);
        subThreadItem item { thrd, ST_TargetWindow, nullptr };
        subThreads.push_back(item);
    } else SetForegroundWindow(s_targetWindowHWND);
    return 0;
}
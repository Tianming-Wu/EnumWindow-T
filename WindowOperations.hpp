// 
// BUG: This File is not valid
//     ERROR Terminating the thread, which causes the program unable to exit, 
// even if I has added it into the thread management service.
// 
#pragma once

#include <windows.h>
#include <thread>

#include "toolset.h"

#define TOOLTIP_TIMER_ID 4
#define WM_OL_RESET WM_USER+21

HWND g_hTooltipWnd;
extern HINSTANCE g_hInstance;
extern HFONT hPublicFont;
//extern HANDLE g_quitevent;
extern bool gb_quitEvent;

bool OutlineWindowWorking;

LRESULT CALLBACK TooltipWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //if(WaitForSingleObject(g_quitevent, 0) == WAIT_OBJECT_0) DestroyWindow(hwnd);
    if(gb_quitEvent) DestroyWindow(hwnd);
    switch (msg)
    {
        case WM_CREATE:
        {
            SetTimer(hwnd, TOOLTIP_TIMER_ID, 3000, nullptr);
            break;
        }
        case WM_TIMER:
        {
            if (wParam == TOOLTIP_TIMER_ID)
            {
                KillTimer(hwnd, TOOLTIP_TIMER_ID);
                DestroyWindow(hwnd);
            }
            break;
        }
        case WM_OL_RESET:
        {
            HWND targetHwnd = (HWND)lParam;
            KillTimer (hwnd, TOOLTIP_TIMER_ID);
            
            RECT windowRect;
            GetWindowRect(targetHwnd, &windowRect);
            MoveWindow(hwnd, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, TRUE);

            SetTimer(hwnd, TOOLTIP_TIMER_ID, 3000, nullptr);
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            // 绘制一个矩形
            HPEN hPen = CreatePen(PS_SOLID, 3, RGB(45, 45, 250));
            SelectObject(hdc, hPen);

            DrawEdge(hdc, &clientRect, EDGE_RAISED, BF_RECT);

            EndPaint(hwnd, &ps);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int ShowWindowOutline_WorkerThread(HWND hwnd)
{
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    OutlineWindowWorking = true;

    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = TooltipWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_StaticOutline{}");

    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);
    RegisterClassExA(&wcex);

    g_hTooltipWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        TEXT("WindowContolExClass_StaticOutline{}"), NULL,
        WS_POPUP,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, NULL, NULL
    );

    //SetLayeredWindowAttributes(g_hTooltipWnd, RGB(0, 0, 0), 100, LWA_COLORKEY | LWA_ALPHA);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    OutlineWindowWorking = false;
    return (int)msg.wParam;
}

[[deprecated("Warning" "This Function did not worked correctly." "It meant to show a border of the target window, but it doesn't do it correctly.")]]
/// @deprecated DO NOT USE IT
void ShowWindowOutline(HWND hwnd) {
    if(!OutlineWindowWorking) {
        std::thread *thrd = new std::thread(ShowWindowOutline_WorkerThread, hwnd);
        subThreadItem item { thrd, ST_OutlineWindow, nullptr };
        subThreads.push_back(item);
    } else {
        SetForegroundWindow(g_hTooltipWnd);
        SendMessage(g_hTooltipWnd, WM_OL_RESET, 0, (LPARAM)hwnd);
    }
}

BOOL IsWindowVisibleOnScreen(HWND hwnd)
{
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    HMONITOR hMonitor = MonitorFromRect(&windowRect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(hMonitor, &monitorInfo);

    RECT intersectRect;
    if (IntersectRect(&intersectRect, &windowRect, &monitorInfo.rcWork))
    {
        return TRUE;
    }
    return FALSE;
}

/*
    事件发送窗口模块
    这个模块可用于向目标窗口发送事件，包括自定义事件。
*/

#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <thread>
#include <vector>

#include "EventSenderWindowIDs.h"
#include "toolset.h"


extern HINSTANCE g_hInstance;
extern HWND hTreeView;
//extern HANDLE g_quitevent;
extern bool gb_quitEvent;
extern HFONT hPublicFont;
extern HWND main_hwnd;

bool EventSenderWindowWindowRunning = false;

LRESULT CALLBACK EventSenderWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    if(gb_quitEvent) DestroyWindow(hwnd);





}

int EventSenderWindow_WorkerThread() {
    HWND eventSenderWindow;
    EventSenderWindowWindowRunning = true;
    
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = EventSenderWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_EventSenderWindow{}");
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassExA(&wcex);

    eventSenderWindow = CreateWindowEx(
        0,
        TEXT("WindowContolExClass_EventSenderWindow{}"), TEXT("事件发送窗口"),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, g_hInstance, NULL
    );
    s_targetWindowHWND = eventSenderWindow;

    subWindows.push_back(eventSenderWindow);

    ShowWindow(eventSenderWindow, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EventSenderWindowWindowRunning = false;
    s_targetWindowHWND = nullptr;

    return (int)msg.wParam;
}


int StartEventSenderWindow() {
    if(!EventSenderWindowWindowRunning) {
        std::thread* thrd = new std::thread(EventSenderWindow_WorkerThread);
        subThreadItem item { thrd, ST_EventSenderWindow, nullptr };
        subThreads.push_back(item);
    } else SetForegroundWindow(s_targetWindowHWND);
    return 0;
}
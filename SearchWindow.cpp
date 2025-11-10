#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <thread>

#include "SearchWindowIDs.hpp"

#include "toolset.h"
#include "config.h"

extern HINSTANCE g_hInstance;
extern HWND hTreeView;
extern bool gb_quitEvent;
extern HFONT hPublicFont;

bool SearchWindowRunning = false;
HWND s_searchWindowHWND = nullptr;

LRESULT CALLBACK SearchWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    enum {
        sw_titleEditHwnd, sw_classEditHwnd,

        sw_SearchBtnHwnd, sw_ClearBtnHwnd,
        SWH_HwndID_MAXCOUNT
    };

    thread_local static HWND ctrls[SWH_HwndID_MAXCOUNT];

    if(gb_quitEvent) DestroyWindow(hwnd);
    switch(uMsg) {
    case WM_CREATE:
    {
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        ctrls[sw_titleEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT,
            55, 25, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);
        ctrls[sw_classEditHwnd] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT /*|ES_AUTOHSCROLL*/,
            55, 55, 255, 24, hwnd, (HMENU)NULL, hInstance, NULL);

        
        for(HWND &i: ctrls) SendMessage(i, WM_SETFONT, WPARAM(hPublicFont), TRUE);

        break;
    }
    case WM_COMMAND:
    {
        switch(HIWORD(wParam)) {
        case BN_CLICKED: // 按下按钮
            switch(LOWORD(wParam))
            {
            case SWH_ID_TREEBTN:
            {
                // HTREEITEM item = TreeView_MapAccIDToHTREEITEM(hTreeView, (WPARAM)g_hTargetWindow);
                // if(item) TreeView_SelectItem(hTreeView, item);
                //     else MessageBox(NULL, "Failed", "ERROR", 0);
                break;
            }
            case SWH_ID_PROPERTYBTN:
            {
                // StartPropertyWindow(g_hTargetWindow);
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
        
        SelectObject(hdc, hPublicFont);
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextStd(hdc, "类名", &posRect(15, 27), DTS_DEFAULT);
        DrawTextStd(hdc, "标题", &posRect(15, 57), DTS_DEFAULT);

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

int SearchWindow_WorkerThread() {
    HWND searchWindow;

    SearchWindowRunning = true;

    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = SearchWindowProc;
    wcex.hInstance = g_hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_SearchWindow{}");
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassExA(&wcex);

    searchWindow = CreateWindowEx(
        /*WS_EX_NOREDIRECTIONBITMAP,*/ 0,
        TEXT("WindowContolExClass_SearchWindow{}"), TEXT("搜索窗口"),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, g_hInstance, NULL
    );
    s_searchWindowHWND = searchWindow;

    subWindows.push_back(searchWindow);

    ShowWindow(searchWindow, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SearchWindowRunning = false;
    s_searchWindowHWND = nullptr;

    return (int)msg.wParam;
}


int StartSearchWindow() {
    if(!SearchWindowRunning) {
        std::thread* thrd = new std::thread(SearchWindow_WorkerThread);
        subThreadItem item { thrd, ST_SearchWindow, nullptr };
        subThreads.push_back(item);
    } else SetForegroundWindow(s_searchWindowHWND);
    return 0;
}
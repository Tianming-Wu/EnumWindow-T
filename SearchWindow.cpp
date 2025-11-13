#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <thread>

#include "SearchWindowIDs.hpp"
#include "PropertyWindow.hpp"

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
        sw_titleEdit, sw_classEdit,
        sw_cbAutoUpdateOnEdit,
        sw_SearchBtn, sw_ClearBtn,
        sw_hwndEdit,
        sw_treeBtn, sw_propertyBtn,
        SWH_HwndID_MAXCOUNT
    };

    thread_local static HWND ctrls[SWH_HwndID_MAXCOUNT];
    thread_local static bool AutoUpdateOnEdit;

    thread_local static HWND targetHwnd;

    if(gb_quitEvent) DestroyWindow(hwnd);
    switch(uMsg) {
    case WM_CREATE:
    {
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        ctrls[sw_classEdit] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT,
            55, 15, 255, 24, hwnd, (HMENU)SWH_EDIT_CLASS, hInstance, NULL);
        ctrls[sw_titleEdit] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP | ES_LEFT /*|ES_AUTOHSCROLL*/,
            55, 45, 255, 24, hwnd, (HMENU)SWH_EDIT_TITLE, hInstance, NULL);

        ctrls[sw_cbAutoUpdateOnEdit] = CreateWindowEx(0, TEXT("Button"), TEXT("自动更新"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | BS_AUTOCHECKBOX,
            15, 79, 90, 25, hwnd, (HMENU)SWH_CB_AUTOUPDATEONEDIT, hInstance, NULL);

        ctrls[sw_SearchBtn] = CreateWindowEx(0, TEXT("Button"), TEXT("搜索"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            185, 79, 90, 25, hwnd, (HMENU)SWH_ID_SEARCHBTN, hInstance, NULL);
        ctrls[sw_ClearBtn] = CreateWindowEx(0, TEXT("Button"), TEXT("清空"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            285, 79, 90, 25, hwnd, (HMENU)SWH_ID_CLEARBTN, hInstance, NULL);

        ctrls[sw_hwndEdit] = CreateWindowEx(0, TEXT("Edit"), NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP  | ES_READONLY|ES_LEFT /*|ES_AUTOHSCROLL*/,
            55, 113, 90, 24, hwnd, (HMENU)NULL, hInstance, NULL);

        ctrls[sw_treeBtn] = CreateWindowEx(0, TEXT("Button"), TEXT("查看列表"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            185, 230, 90, 25, hwnd, (HMENU)SWH_ID_TREEBTN, hInstance, NULL);
        ctrls[sw_propertyBtn] = CreateWindowEx(0, TEXT("Button"), TEXT("查看属性"), WS_CHILD|WS_VISIBLE|WS_TABSTOP | WS_DISABLED | BS_FLAT,
            285, 230, 90, 25, hwnd, (HMENU)SWH_ID_PROPERTYBTN, hInstance, NULL);
        
        for(HWND &i: ctrls) SendMessage(i, WM_SETFONT, WPARAM(hPublicFont), TRUE);

        AutoUpdateOnEdit = Config.SearchWindow.AutoUpdateOnEdit;
        SendMessage(ctrls[sw_cbAutoUpdateOnEdit], BM_SETCHECK, AutoUpdateOnEdit?BST_CHECKED:BST_UNCHECKED, 0);
        
        break;
    }
    case WM_COMMAND:
    {
        switch(HIWORD(wParam)) {
        case BN_CLICKED: { // 按下按钮
            switch(LOWORD(wParam)) {
            case SWH_ID_TREEBTN: {
                HTREEITEM item = TreeView_MapAccIDToHTREEITEM(hTreeView, (WPARAM)targetHwnd);
                if(item) TreeView_SelectItem(hTreeView, item);
                    else MessageBox(NULL, "Failed", "ERROR", 0);
                break;
            }
            case SWH_ID_PROPERTYBTN: {
                StartPropertyWindow(targetHwnd);
                break;
            }
            case SWH_ID_SEARCHBTN: {
                std::string className = _GetWindowText(ctrls[sw_classEdit]);
                std::string wndTitle = _GetWindowText(ctrls[sw_titleEdit]);

                targetHwnd = FindWindow(className.c_str(), wndTitle.c_str());
                if(targetHwnd != NULL) {
                    std::stringstream tsstr; std::string tstr;
                    tsstr << std::hex << targetHwnd; tstr = tsstr.str(); tsstr.str("");
                    tstr = tstr.substr(tstr.length()-6); tsstr << "0x" << tstr;
                    SetWindowText(ctrls[sw_hwndEdit], tsstr.str().c_str()); // 显示窗口句柄（16进制）
                    tsstr.str("");

                    EnableWindow(ctrls[sw_treeBtn], TRUE);
                    EnableWindow(ctrls[sw_propertyBtn], TRUE);
                } else {
                    SetWindowText(ctrls[sw_hwndEdit], "");

                    EnableWindow(ctrls[sw_treeBtn], FALSE);
                    EnableWindow(ctrls[sw_propertyBtn], FALSE);
                }
                break;
            }
            case SWH_ID_CLEARBTN: {
                SetWindowText(ctrls[sw_classEdit], "");
                SetWindowText(ctrls[sw_titleEdit], "");
                
                break;
            }
            case SWH_CB_AUTOUPDATEONEDIT: {
                AutoUpdateOnEdit = (SendMessage(ctrls[sw_cbAutoUpdateOnEdit], BM_GETCHECK, 0, 0) == BST_CHECKED);
                Config.SearchWindow.AutoUpdateOnEdit = AutoUpdateOnEdit;
                break;
            }}
            break;
        }
        case EN_CHANGE: { // 编辑框修改
            WORD control = LOWORD(wParam);
            // Search boxes
            if(control == SWH_EDIT_TITLE || control == SWH_EDIT_CLASS) { // 修改的是搜索框（而不是结果显示）
                std::string className = _GetWindowText(ctrls[sw_classEdit]);
                std::string wndTitle = _GetWindowText(ctrls[sw_titleEdit]);

                if(className.empty() && wndTitle.empty()) {
                    EnableWindow(ctrls[sw_SearchBtn], FALSE);
                    EnableWindow(ctrls[sw_ClearBtn], FALSE);

                    SetWindowText(ctrls[sw_hwndEdit], "");
                } else { // 不同时为空
                    EnableWindow(ctrls[sw_SearchBtn], TRUE);
                    EnableWindow(ctrls[sw_ClearBtn], TRUE);

                    if(AutoUpdateOnEdit) { // 自动搜索
                        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(SWH_ID_SEARCHBTN, BN_CLICKED), (LPARAM)ctrls[sw_SearchBtn]);
                    }
                }
            }
            
            break;
        }}
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        SelectObject(hdc, hPublicFont);
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextStd(hdc, "类名", &posRect(15, 17), DTS_DEFAULT);
        DrawTextStd(hdc, "标题", &posRect(15, 47), DTS_DEFAULT);
        DrawTextStd(hdc, "hwnd", &posRect(15, 115), DTS_DEFAULT);

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
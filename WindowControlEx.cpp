#include "WindowControlEx.h"
#include "config.h"

#include "RMenuProcessor.hpp"
#include "ScanThread.hpp"

HWND hTreeView, _hStatusBar;
HWND main_hwnd;
HMENU hPopupMenu, hWindowMenu;
HFONT hPublicFont;

//HANDLE g_quitevent;
bool gb_quitEvent = false;

HINSTANCE g_hInstance;

bool Scanning;
std::thread *scanthrd;
int windowcount, filteredwindowcount;
HTREEITEM tree_conduct;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpWCmdLine, INT nCmdShow)
{
    g_hInstance = hInstance;
    InitCommonControls();
    InitPropertyWindow();

    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass");

    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassEx(&wcex);

    hWindowMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDW_MENU));

    main_hwnd = CreateWindowEx(0, 
        TEXT("WindowContolExClass"), TEXT("WindowControl Ex"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 550, 550,
        NULL, hWindowMenu, hInstance, NULL
    );

    ShowWindow(main_hwnd, nCmdShow);

    hPublicFont = CreateFont(19, 0, 0, 0,
        FW_MEDIUM, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
        "Segoe UI"
    );

    SendMessage(main_hwnd, WM_SETFONT, WPARAM(hPublicFont), TRUE);
    
    BlockList.load(); // 加载黑名单
    Config.load(); // 加载配置

    //g_quitevent = CreateEventA(NULL, FALSE, FALSE, NULL); // 配置全局线程退出信号

    BeginScan();

    SendMessage(main_hwnd, WM_INITIALIZE, 0, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) // 主消息循环
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //SetEvent(g_quitevent); // 发送全局子进程进程退出消息

    BlockList.save(); // 保存黑名单
    Config.save(); // 保存配置

    gb_quitEvent = true;
    //MessageBox(NULL, "Event Sent", "DEBUG", MB_OK);

    CloseAllSubWindows();
    //MessageBox(NULL, "Windows Closed", "DEBUG", MB_OK);
    CloseAllThreads(); // 等待所有子线程退出
    //MessageBox(NULL, "Threads Closed", "DEBUG", MB_OK);
    if(scanthrd->joinable()) scanthrd->join();
    //MessageBox(NULL, "Scan Thread Closed", "DEBUG", MB_OK);

    //std::terminate();

    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        hTreeView = CreateWindowEx(0, WC_TREEVIEW, TEXT("Tree View"), 
            WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS,
            20, 20, 200, 300, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
        );
        _hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,
			hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        constexpr int StatusBarParts = 3;
        //1: 状态 / 2: 窗口尺寸 / 3: 窗口数
        int paParts[StatusBarParts] = { 50, 170, 130 };
        MakeSizeGrip(paParts, StatusBarParts);
        SendMessage(_hStatusBar, SB_SETPARTS, (WPARAM) StatusBarParts, (LPARAM) paParts);

        SendMessage(_hStatusBar, SB_SETTEXT, 0, (LPARAM)"初始化");
        SendMessage(_hStatusBar, SB_SETTEXT, 2, (LPARAM)"初始化");

        // Load the right click menu
        HMENU hRightMenu = LoadMenu(((LPCREATESTRUCT)lParam)->hInstance, /*szMenuName*/ MAKEINTRESOURCE(IDM_MAINMENU));
        hPopupMenu = GetSubMenu(hRightMenu, 0);
    }
    break;
    case WM_ADDROOT: {
        MsgDataPackA* data = reinterpret_cast<MsgDataPackA*>(lParam);
        tree_conduct = AddRootItem(hTreeView, data->Name.c_str(), (LPARAM)data->handle);
        SetEvent(reinterpret_cast<HANDLE>(data->event));
        break;
    }
    case WM_ADDCHILD: {
        MsgDataPackB* data = reinterpret_cast<MsgDataPackB*>(lParam);
        tree_conduct = AddChildItem(hTreeView, data->parent, data->Name.c_str(), (LPARAM)data->handle);
        SetEvent(reinterpret_cast<HANDLE>(data->event));
        break;
    }
    case WM_RESET: {
        TreeView_DeleteAllItems(hTreeView);
        SetEvent(reinterpret_cast<HANDLE>(lParam));
        break;
    }
    case WM_CONTEXTMENU: {  
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, NULL);
        break;
    }
    case WM_INITIALIZE:
    {
        CheckMenuItem(hWindowMenu, IDW_BLACKLIST_ENABLED, Config.EnableBlockList?(MF_CHECKED):(MF_UNCHECKED));

        CheckMenuItem(hWindowMenu, IDW_AUTOUPDATE, Config.PropertyWindow.AutoUpdateEnabled?(MF_CHECKED):(MF_UNCHECKED));
        if(!Config.PropertyWindow.AutoUpdateEnabled)
            for(u_int i = IDW_AUINTERVAL50MS; i <= IDW_AUINTERVALCUSTOM; i++)
                EnableMenuItem(hWindowMenu, i, MF_BYCOMMAND | MF_DISABLED);
        UINT updateInterval_Handle;
        switch(Config.PropertyWindow.AutoUpdateInterval) {
        case 50  : updateInterval_Handle = IDW_AUINTERVAL50MS;   break;
        case 100 : updateInterval_Handle = IDW_AUINTERVAL100MS;  break;
        case 500 : updateInterval_Handle = IDW_AUINTERVAL500MS;  break;
        case 1000: updateInterval_Handle = IDW_AUINTERVAL1000MS; break;
        case 5000: updateInterval_Handle = IDW_AUINTERVAL5000MS; break;
        default: {
            updateInterval_Handle = IDW_AUINTERVALCUSTOM;
            std::string p = std::to_string(Config.PropertyWindow.AutoUpdateInterval) + "ms";

            SetMenuText(hWindowMenu, IDW_AUINTERVALCUSTOM, p);
            //EnableMenuItem(hWindowMenu, IDW_AUINTERVALCUSTOM, MF_BYCOMMAND | MF_ENABLED);
            DrawMenuBar(hwnd);
            break;
        }
        }
        CheckMenuItem(hWindowMenu, updateInterval_Handle, MF_CHECKED);

        HMENU hSubMenu = GetSubMenu(hWindowMenu, 4); // 加载子菜单项
        EnableMenuItem(hSubMenu, 3, MF_BYPOSITION | (Config.PropertyWindow.AutoUpdateEnabled?(MF_ENABLED):(MF_DISABLED)) ); // 禁用设置

        break;
    }
    case WM_COMMAND: 
        return RMenuProcessor(hwnd, uMsg, wParam, lParam);
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostMessage(hwnd, WM_QUIT, 0, 0);
        break;
    case WM_QUIT:
        break;
    case WM_SIZE: {
        RECT wndRect;
        GetWindowRect(hwnd, &wndRect);
        MoveWindow(hTreeView, 0, 21, wndRect.right-wndRect.left-15, wndRect.bottom-wndRect.top-125, TRUE);
        //MoveWindow(_hStatusBar, 0, 0, 0, 0, TRUE); // 确保状态栏处在正确的位置
        SendMessage(_hStatusBar, WM_SIZE, 0, 0); // 这样也可以工作

        std::stringstream sstr;
        sstr << "size: " << std::setw(4) << std::right << wndRect.left
             << ", " << std::setw(4) << std::right << wndRect.top
             << ", " << std::setw(4) << std::right << wndRect.right - wndRect.left
             << ", " << std::setw(4) << std::right << wndRect.bottom - wndRect.top;
        SendMessage(_hStatusBar, SB_SETTEXT, 1, (LPARAM)sstr.str().c_str());
        break;
    }
    case WM_MOVE: {
        RECT wndRect;
        GetWindowRect(hwnd, &wndRect);
        std::stringstream sstr;
        sstr << "size: " << std::setw(4) << std::right << wndRect.left
             << ", " << std::setw(4) << std::right << wndRect.top
             << ", " << std::setw(4) << std::right << wndRect.right - wndRect.left
             << ", " << std::setw(4) << std::right << wndRect.bottom - wndRect.top;
        SendMessage(_hStatusBar, SB_SETTEXT, 1, (LPARAM)sstr.str().c_str());
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT wndRect, sizeRect;
        GetClientRect(hwnd, &wndRect);
        sizeRect.bottom = wndRect.bottom - wndRect.top; sizeRect.right = wndRect.right-wndRect.left;

        std::stringstream sstr;
        
        SelectObject(hdc, hPublicFont);
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetBkMode(hdc, TRANSPARENT);

        DrawTextStd(hdc, "Produced by Tianming", &posRect(2,2), DT_LEFT|DT_SINGLELINE|DT_NOCLIP);
        
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 350; //261*334: Actual Size
        mmi->ptMinTrackSize.y = 320; 
        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

HTREEITEM AddChildItem(HWND hwndTree, HTREEITEM hParent, LPCTSTR lpszItem, LPARAM lParam)
{
    TVINSERTSTRUCT tvInsert;
    tvInsert.hParent = hParent;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvInsert.item.pszText = (LPTSTR)lpszItem;
    tvInsert.item.lParam = lParam;
    return TreeView_InsertItem(hwndTree, &tvInsert);
}
HTREEITEM AddRootItem(HWND hwndTree, LPCTSTR lpszItem, LPARAM lParam)
{
    TVINSERTSTRUCT tvInsert;
    tvInsert.hParent = TVI_ROOT;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvInsert.item.pszText = (LPTSTR)lpszItem;
    tvInsert.item.lParam = lParam;
    return TreeView_InsertItem(hwndTree, &tvInsert);
}


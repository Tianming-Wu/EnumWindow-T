/*
    PluginLoaderWindow 插件加载窗口实现文件。

    已知问题：目前这个模块没有整合进线程管理器，会导致插件注入窗口无法正确被主线程关闭。
    需要重构线程管理器来允许这个外部 dll 将自己添加进来。

    或者，保证它正确响应 g_quitEvent 来自行关闭，但是理论上 quitEvent 无法传递到这里。
*/



#include <Windows.h>


#include <thread>

LRESULT CALLBACK PluginLoaderWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{






}


int PluginLoaderWindow_WorkerThread() {
    HWND pluginLoaderWindow;
    
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = PluginLoaderWindowProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = TEXT("WindowContolExClass_PluginLoaderWindow{}");
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW);

    RegisterClassEx(&wcex);

    pluginLoaderWindow = CreateWindowEx(
        0,
        TEXT("WindowContolExClass_PluginLoaderWindow{}"), TEXT("插件加载窗口"),
        WS_OVERLAPPEDWINDOW &~ WS_SIZEBOX &~ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    ShowWindow(pluginLoaderWindow, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int StartPluginLoaderWindow_impl() {
    std::thread workerThread(PluginLoaderWindow_WorkerThread);
    workerThread.detach();
    return 0;
}
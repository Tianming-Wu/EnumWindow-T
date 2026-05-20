#include "toolset.h"
#include <shellapi.h>
#include <winternl.h>
#include <functional>


BOOL TextOutStd(HDC _hdc, int x, int y, std::string _str)
{
    return TextOut(_hdc, x, y, _str.c_str(), (int)_str.length());
}

int DrawTextStd(HDC hdc, std::string Text, LPRECT lprc, UINT format, bool Repaint)
{
    DWORD ret = DrawText(hdc, Text.c_str(), (int)Text.length(), lprc, format);
    if(Repaint && ret) {
        RECT rc = *lprc;
        rc.bottom = rc.top + ret;

    }
    return ret;
}

RECT makeRectAbsolute(int t, int b, int l, int r)
{
    RECT rect;
    rect.top = t, rect.bottom = b, rect.left = l, rect.right = r;
    return rect;
}

RECT makeRectRelative(int x, int y, int h, int w)
{
    RECT rect;
    rect.top = y, rect.bottom = y+h, rect.left = x, rect.right = x+w;
    return rect;
}

RECT posRect(int x, int y) {
    RECT rect;
    rect.top = y, rect.bottom = y+1, rect.left = x, rect.right = x+1;
    return rect;
}

void MakeSizeGrip(int* _array, int Size) {
    int _t = 0;
    for(int i = 0; i < Size; i++) {
        _array[i] += _t;
        _t = _array[i];
    }
}

HWND GetItemHandle(HWND hTreeView) {
    HTREEITEM hFocusedItem = TreeView_GetNextItem(hTreeView, NULL, TVGN_CARET);
    if (hFocusedItem) {
        TVITEM item;
        item.mask = TVIF_PARAM;
        item.hItem = hFocusedItem;
        TreeView_GetItem(hTreeView, &item);
        HWND hwndHandle = reinterpret_cast<HWND>(item.lParam);
        return hwndHandle;
    } else return nullptr;
}

bool SetMenuText(HMENU hMenu, int uid, std::string Text)
{
    MENUITEMINFO mii { sizeof(MENUITEMINFO) };
    GetMenuItemInfo(hMenu, uid, FALSE, &mii);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.cch = (DWORD)Text.length();
    mii.dwTypeData = const_cast<LPSTR>(Text.c_str());

    return SetMenuItemInfo(hMenu, uid, FALSE, &mii);
}

void SetMenuSelection(HMENU hMenu, int range_l, int range_r, int active) {
    for(int i = range_l; i <= range_r; i++) {
        CheckMenuItem(hMenu, i, (i==active)?(MF_CHECKED):(MF_UNCHECKED));
        //EnableMenuItem(hMenu, i, MF_BYCOMMAND | ((i==active)?(MF_ENABLED):(MF_DISABLED)));
    }
}

std::string GetWindowClass(HWND hwnd)
{
    char className[256];
    GetClassName(hwnd, className, sizeof(className));

    return std::string(className);
}

std::string _GetWindowText(HWND hwnd)
{
    // int size = GetWindowTextLength(hwnd);
    // std::string result; result.resize(size+1);
    // GetWindowText(hwnd, (LPSTR)result.data(), size+1);

    int size = GetWindowTextLength(hwnd);
    char *buf = new char[size + 1];
    GetWindowText(hwnd, buf, size + 1);
    std::string result(buf);
    delete[] buf;
    return result;
}

std::vector<subThreadItem> subThreads;
std::vector<subWindowItem> subWindows;

void CloseAllThreads() {
    for(auto& item : subThreads) {
        //if(item.type == ST_PropertyThread) DestroyWindow(item.hwnd);
        //static int c = 0;
        if(item.handle->joinable()) {
            //MessageBox(NULL, (std::string("Thrd Join ")+std::to_string(c++)).c_str(), "DEBUG 2", MB_OK);
            //item.handle->join(); // Why does this block?
            
        } 
    }
}

void CloseAllSubWindows() {
    for(auto& item : subWindows) {
        if(IsWindow(item.hwnd)) DestroyWindow(item.hwnd);
    }
}

// 递归查找 TreeView 项，按 lParam 匹配 accID（通常保存 HWND）
static HTREEITEM FindItemByParam(HWND hTreeView, HTREEITEM hItem, HWND hwndTarget)
{
    if(!hItem) return NULL;
    TVITEM tvi { 0 };
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hItem;
    if(TreeView_GetItem(hTreeView, &tvi)) {
        if((HWND)(tvi.lParam) == hwndTarget) return hItem;
    }

    HTREEITEM child = TreeView_GetChild(hTreeView, hItem);
    for(HTREEITEM cur = child; cur; cur = TreeView_GetNextSibling(hTreeView, cur)) {
        HTREEITEM found = FindItemByParam(hTreeView, cur, hwndTarget);
        if(found) return found;
    }
    return NULL;
}

HTREEITEM FindTreeViewItemByHwnd(HWND hTreeView, HWND hwndTarget)
{
    if(!IsWindow(hTreeView)) return NULL;
    HTREEITEM root = TreeView_GetRoot(hTreeView);
    for(HTREEITEM cur = root; cur; cur = TreeView_GetNextSibling(hTreeView, cur)) {
        HTREEITEM found = FindItemByParam(hTreeView, cur, hwndTarget);
        if(found) return found;
    }
    return NULL;
}

bool FocusControlAcrossThreads(HWND controlHwnd)
{
    if(!IsWindow(controlHwnd)) return false;

    HWND rootWindow = GetAncestor(controlHwnd, GA_ROOT);
    if(!IsWindow(rootWindow)) rootWindow = controlHwnd;

    DWORD targetThreadId = 0;
    DWORD currentThreadId = GetCurrentThreadId();
    targetThreadId = GetWindowThreadProcessId(rootWindow, nullptr);

    bool attached = false;
    if(targetThreadId && targetThreadId != currentThreadId) {
        attached = AttachThreadInput(currentThreadId, targetThreadId, TRUE) != 0;
    }

    SetForegroundWindow(rootWindow);
    SetActiveWindow(rootWindow);
    SetFocus(controlHwnd);

    if(attached) {
        AttachThreadInput(currentThreadId, targetThreadId, FALSE);
    }
    return true;
}

bool IsProcessElevated()
{
    HANDLE token = nullptr;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation;
    DWORD retSize = 0;
    BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &retSize);
    CloseHandle(token);
    if(!ok) return false;
    return elevation.TokenIsElevated != 0;
}

std::string TranslateLastError() {
    DWORD errorCode = GetLastError();
    if (errorCode == 0) return ""; // 成功，没有错误消息。
    LPSTR messageBuffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr
    );

    std::string result;
    if (size > 0 && messageBuffer != nullptr) {
        result.assign(messageBuffer, size);
        while (!result.empty() &&  (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
    } else result = "";
    if (messageBuffer != nullptr) LocalFree(messageBuffer);
    return result;
}

fs::path getExecutableDir() {
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) != 0) {
        std::string pathStr = std::string(exePath);
        size_t lastSlash = pathStr.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return pathStr.substr(0, lastSlash);
        }
    }
    return ""; // 获取路径失败，返回空字符串。
}
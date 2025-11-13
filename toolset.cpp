#include "toolset.h"


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
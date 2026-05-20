# 插件系统

>[!NOTICE]
> 需要事先说明的一点是，插件并不是给 WindowControlEx 制作的。目前没有任何给 WindowControlEx 添加插件加载功能的计划。
> 插件实际上是用于注入的 DLL，可以注入到目标进程中，并提供一些实用功能。

插件系统区别于主项目的重要特征是“侵入性”。主项目的任何行为均不会对目标进程造成任何实质性的影响（或者可以被认为是攻击的行为）。插件实际上被注入到目标进程中，并可以对目标进程造成更大的影响。

目前插件安全性不做保证，请在加载前注意辨别。此程序的注入是“通用”的，你可以加载其他人（可能使用其他语言）编写的任何注入式DLL作为插件，只要它的行为由“被加载”事件触发。所以理论上，你应该可以用此功能注入类似变速齿轮等工具的DLL。



## 插件开发

有两种可选的路线：
第一种是“通用”的，可以使用其他工具加载。插件的行为需要由“被加载”事件触发。
第二种是“专用”的，必须使用本工具加载。你需要按照定义编写插件。

### 通用插件
插件的行为需要由“被加载”事件触发，而不是由调用某个特定的函数触发。因为目标进程中几乎 100% 没有调用相关插件功能的代码，除非你的插件是一个“替代”类型的插件。由于这类插件的特殊性，你需要转为使用 [InjectorDLL 项目](https://github.com/Tianming-Wu/InjectorDLL) 注入，而不是本工具。


以下是一个使用 C/C++ 语言编写的最简插件示例：
```cpp
#include <windows.h>

HMODULE g_hInstance = NULL;

int SomeAction() {
    MessageBoxA(NULL, "注入成功！", "插件", MB_OK);
    return 0;
}

DWORD WINAPI PluginWorkerThread(LPVOID lpParam) {
    // 1. 在这里执行真正的初始化，比如 Hook、加载配置文件、初始化网络等
    // 2. 此时已经脱离了 Loader Lock 的限制，可以安全调用绝大多数 Windows API
    SomeAction();
    return 0;
}

int APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hinstDLL;

        // 禁用线程库调用通知，减少不必要的 DLL_THREAD_ATTACH 和 DLL_THREAD_DETACH 通知
        DisableThreadLibraryCalls(hModule);

        // 注意：你不应在 DllMain 中进行任何可能导致 Loader Lock 的操作，包括等待线程初始化，加载其他 DLL。
        // 你必须立刻创建独立线程，然后让 DllMain 闪退（快速返回 TRUE）。不要等待线程完成初始化，否则会导致死锁。
        // 被创建的线程会在脱离 Loader Lock 后执行真正的初始化工作。
        HANDLE hThread = CreateThread(NULL, 0, PluginWorkerThread, hModule, 0, NULL);
        if (hThread) {
            CloseHandle(hThread); // 关闭句柄，防止句柄泄漏
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
```


### 专用插件

专用插件结构更简单，需要操心的问题更少。但是只能由本工具加载，因此需要使用 C/C++ 语言编写。

你需要按照以下基础结构定义插件：
```cpp

#include <windows.h>
#include <thread>

#include "plugin.hpp"

CLAIM_PLUGIN_ENTRYSUPPORT

PLUGIN_ENTRY void PluginMain(void* lpParam) {
    // 在这里解包参数结构体（如果需要），执行插件的主要功能。
}

```


## 插件元数据

插件的元数据文件不是必须的，但是如果你希望插件在加载的时候可以传参（只对于专用插件），那么你需要元数据文件来定义参数。

如果你的插件是通用插件，则应该更改 `Type` 为 `generic`，并且不包含 Parameter 节。

即使你的插件是专用插件，你也可以选择不包含 Parameter 节，插件将被视作零参数插件，lpParam 将为 NULL。

元数据文件是一个与插件 DLL 同名的 .meta 文件，位于同一目录下。它是一个文本文件，使用 INI 格式。

示例 (`TinyTitleBar.meta`)：
```ini
[Plugin]
Type=specialized
Name=TinyTitleBar
Author=Tianming Wu
Description=给窗口显示一个微型标题栏，以占用尽可能小的屏幕空间
Version=1.0.0.1

[Parameter]
Count=1
Param1=auto:hwnd|HWND|0|目标窗口的句柄

```

对于 `Parameter` 节，`Count` 定义了参数的数量，`Param1`、`Param2` 等定义了每个参数的详细信息。参数定义的格式如下：

`ParamN=name|datatype|defaultvalue|description`

- name: 参数名称，会显示在用户界面中，用户可以修改它的值。也可以使用特定前缀来控制参数的显示和行为。
- datatype: 参数的数据类型，目前支持 `int`、`float`、`string`、`HWND`、`DWORD` 等常见类型。
- defaultvalue: 参数的默认值，必须与数据类型匹配。如果你在 name 中使用 auto 前缀，需要留空。
- description: 参数的描述信息，会显示在用户界面中，帮助用户理解这个参数的作用。

其中 name 可以使用以下前缀：

- `hide`: 该参数不会显示在插件加载界面中，而是使用默认值。用户可以通过点击加载时按住 Shift 显示隐藏的参数。
- `reserved`: 该参数总会使用默认值。
- `auto`: 该参数会自动从预定义的参数列表中获取值。列表可在下方查询。类型必须与列表中的类型匹配，否则插件会被拒绝加载。需要留空 defaultvalue 项。

**预定义参数列表**：
- `hwnd`: 目标窗口的句柄，类型为 HWND。
- `pid`: 目标进程的 PID，类型为 DWORD。

如果插件只申请了一个参数，则接收到的 lpParam 指针会直接指向该参数。

如果插件申请了多个参数，则 lpParam 指向一个参数结构体，按顺序拼接了所有参数。注意拼接过程不处理对齐，所以你在dll中定义参数结构体时需要使用类似以下定义方式：
```cpp
#pragma pack(push, 1)
struct myParams {
    HWND hwnd;
    DWORD pid;
};
#pragma pack(pop)
```

对于 string 类型，lpParam 或者结构体的对应位置会是一个指向字符串的指针。你释放 lpParam 的时候不会释放它，你需要自己释放它（通过 free()。），插件加载器不会负责释放它。
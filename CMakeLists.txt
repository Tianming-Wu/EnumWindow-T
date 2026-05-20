cmake_minimum_required(VERSION 3.10)

project(
    WindowControlEx
    VERSION 1.3.1
    DESCRIPTION "The Real GUI Version of the classic WindowControl Commandline project, all in Win32API."
    LANGUAGES CXX
)

option(BUILD_PLUGIN_SYSTEM "Build the plugin subsystem" ON)

set(PROJECT_SOURCES
    WindowControlEx.cpp WindowControlEx.h
    ScanThread.hpp ScanThread.cpp
    RuleSet.hpp RuleSet.cpp
    toolset.h toolset.cpp
    config.h config.cpp
    PropertyWindow.hpp PropertyWindowIDs.h PropertyWindow.cpp
    TargetWindow.hpp TargetWindowIDs.h TargetWindow.cpp
    SearchWindow.hpp SearchWindowIDs.hpp SearchWindow.cpp
    RMenuProcessor.hpp RMenuProcessor.cpp
    WindowOperations.hpp
    menu.rc menu_items.h
    about.hpp about.cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:GBK>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:GBK>")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/execution-charset:GBK>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/execution-charset:GBK>")
endif()

option(JSONCPP_WITH_TESTS "Build jsoncpp tests" OFF)

include(FetchContent)
FetchContent_Declare(
  jsoncpp
  GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
  GIT_TAG        1.9.5  # or latest stable
)

FetchContent_MakeAvailable(jsoncpp)

add_executable(WindowControlEx ${PROJECT_SOURCES})
target_link_libraries(WindowControlEx comctl32 jsoncpp_static psapi)
set_target_properties(WindowControlEx PROPERTIES WIN32_EXECUTABLE TRUE)


if(BUILD_PLUGIN_SYSTEM)
    add_library(PluginLoader SHARED
        Plugins/PluginLoader.cpp
    )
    target_link_libraries(PluginLoader comctl32 psapi)
endif()
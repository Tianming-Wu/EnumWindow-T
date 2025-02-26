cmake_minimum_required(VERSION 3.5)

project(
    WindowControlEx
    VERSION 1.3.1
    DESCRIPTION "The Real GUI Version of the classic WindowControl Commandline project, all in Win32API."
    LANGUAGES CXX
)

set(PROJECT_SOURCES
    WindowControlEx.cpp WindowControlEx.h
    ScanThread.hpp
    blocklist_fx.h blocklist_fx.cpp
    toolset.h toolset.cpp
    config.h config.cpp
    PropertyWindow.hpp PropertyWindowIDs.h
    FindWindow.hpp FindWindowIDs.h
    RMenuProcessor.hpp
    WindowOperations.hpp
    menu.rc menu_items.h
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(JSONCPP_WITH_TESTS "Build jsoncpp tests" OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/jsoncpp-master)
include_directories(${CMAKE_SOURCE_DIR}/jsoncpp-master/include)

add_executable(WindowControlEx ${PROJECT_SOURCES})

target_link_libraries(WindowControlEx comctl32 jsoncpp_static psapi)

set_target_properties(WindowControlEx PROPERTIES WIN32_EXECUTABLE TRUE)
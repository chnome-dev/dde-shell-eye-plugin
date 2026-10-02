# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# deepin V20 (dde-dock 5.x) 版卡通眼珠插件构建脚本。
#
# V20 的 dde-dock 是 Qt5 + QWidget 体系，只认 PluginsItemInterface（V1），
# 因此这里用纯 Qt Widgets 实现（不依赖 Qt Quick / QML 运行时模块）。
#
# 依赖：qtbase5-dev（Qt5 Core/Gui/Widgets/Network）+ dde-dock-dev（接口头文件）
#
# 用法：
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j$(nproc)
# 产物：build/libcartoon-eye.so
#
# 交叉编译时额外指定：
#   -DDDE_DOCK_INCLUDE_DIR=/path/to/dde-dock/interfaces
#   -DCMAKE_PREFIX_PATH=/path/to/qt5-sysroot/usr

cmake_minimum_required(VERSION 3.16)
project(cartoon-eye-v20 VERSION 1.9.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ---- 依赖：Qt5（Widgets 体系，不需要 Quick/Qml） ----
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)

# ---- 依赖：dde-dock 插件接口头（PluginsItemInterface，V1） ----
if(NOT DDE_DOCK_INCLUDE_DIR)
    find_path(DDE_DOCK_INCLUDE_DIR
        NAMES pluginsiteminterface.h
        PATHS /usr/include/dde-dock /usr/include
    )
endif()

if(NOT DDE_DOCK_INCLUDE_DIR)
    message(FATAL_ERROR
        "未找到 dde-dock 插件接口头 pluginsiteminterface.h。\n"
        "请先安装 dde-dock-dev：sudo apt install dde-dock-dev")
endif()
message(STATUS "dde-dock 接口头目录: ${DDE_DOCK_INCLUDE_DIR}")

# ---- 源文件 ----
set(SOURCES
    eyeplugin.cpp
    eyeplugin.h
    eyeartist.cpp
    eyeartist.h
    eyestyle.h
    eyewidget.cpp
    eyewidget.h
    eyemenu.cpp
    eyemenu.h
    eyepopup.cpp
    eyepopup.h
)

add_library(cartoon-eye MODULE ${SOURCES})

# plugin.json 供 Q_PLUGIN_METADATA(IID ... FILE "plugin.json") 读取
target_include_directories(cartoon-eye PRIVATE
    ${DDE_DOCK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/package
)

target_link_libraries(cartoon-eye PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

set_target_properties(cartoon-eye PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "cartoon-eye"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS cartoon-eye LIBRARY DESTINATION lib/dde-dock/plugins)

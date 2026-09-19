cmake_minimum_required(VERSION 3.16)
project(ds-eye VERSION 1.9.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network)
find_package(DDEShell REQUIRED)
find_package(DDEShellDock REQUIRED)

add_library(ds-eye SHARED
    eyeapplet.h
    eyeapplet.cpp
)
target_link_libraries(ds-eye PRIVATE Dde::Shell Dde::ShellDock Qt6::Gui Qt6::Network)

ds_install_package(PACKAGE org.deepin.ds.dock.eye TARGET ds-eye)

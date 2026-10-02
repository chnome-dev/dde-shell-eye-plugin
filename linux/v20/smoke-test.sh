#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# dde-dock 插件加载冒烟测试的跑法：用 sysroot 的 Qt5 编译并离屏运行
# loader-smoke-test.cpp，复刻 dde-dock 的加载关卡。
#
# 用法：
#   bash smoke-test.sh                      # 测 prebuilt/libcartoon-eye.so
#   bash smoke-test.sh /path/to/other.so
#   V20_SYSROOT=... bash smoke-test.sh

set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSROOT="${V20_SYSROOT:-$HOME/.cache/dde-shell-eye-plugin/v20-sysroot}"
SO="${1:-$ROOT/prebuilt/libcartoon-eye.so}"

QINC="$SYSROOT/usr/include/x86_64-linux-gnu/qt5"
[ -d "$QINC/QtWidgets" ] || QINC="$SYSROOT/usr/include/qt5"
DDEINC="$SYSROOT/usr/include/dde-dock"
QLIB="$SYSROOT/usr/lib/x86_64-linux-gnu"
PLUGDIR="$QLIB/qt5/plugins/platforms"

[ -f "$SO" ] || { echo "!! 找不到 $SO" >&2; exit 1; }
[ -d "$QINC/QtWidgets" ] || { echo "!! 找不到 Qt5 头，请先跑 setup-sysroot.sh" >&2; exit 1; }
[ -f "$DDEINC/pluginsiteminterface.h" ] || { echo "!! 找不到 dde-dock 头，请先跑 setup-sysroot.sh" >&2; exit 1; }

OBJ="$ROOT/build-cross"
mkdir -p "$OBJ"

echo "==> 编译冒烟测试 ..."
g++ -fPIC -std=c++14 -O1 -o "$OBJ/loader-smoke" "$ROOT/loader-smoke-test.cpp" \
    -I"$QINC" -I"$QINC/QtCore" -I"$QINC/QtGui" -I"$QINC/QtWidgets" -I"$DDEINC" \
    -L"$QLIB" -Wl,-rpath-link,"$QLIB" \
    -lQt5Core -lQt5Gui -lQt5Widgets

echo "==> 离屏运行 ..."
# QT_LOGGING_RULES：qInfo 默认被过滤掉，显式打开才能看到测试输出
QT_LOGGING_RULES="*=true" \
QT_QPA_PLATFORM=offscreen \
QT_QPA_PLATFORM_PLUGIN_PATH="$PLUGDIR" \
LD_LIBRARY_PATH="$QLIB:$PLUGDIR" \
"$OBJ/loader-smoke" "$SO"
echo
echo "==> 冒烟测试通过"

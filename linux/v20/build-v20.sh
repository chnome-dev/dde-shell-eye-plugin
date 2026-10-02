#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 在 deepin V20 机器上原生编译 dde-dock 版卡通眼珠插件。
#
# 用法：bash build-v20.sh
# 产物：v20/build/libcartoon-eye.so（dist/make-dual-deb.sh 会自动找到它）
#
# 注意：V20 版不依赖 Qt Quick / QML，只要 qtbase5-dev（Core/Gui/Widgets/Network）
# 和 dde-dock 的插件接口头。

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- 1) 编译工具 ----
echo "==> 检查编译工具 ..."
missing=""
command -v cmake >/dev/null 2>&1 || missing="$missing cmake"
command -v g++   >/dev/null 2>&1 || missing="$missing build-essential"
if ! dpkg -s qtbase5-dev >/dev/null 2>&1 \
   && [ ! -f /usr/include/x86_64-linux-gnu/qt5/QtWidgets/QWidget ] \
   && [ ! -f /usr/include/qt5/QtWidgets/QWidget ]; then
    missing="$missing qtbase5-dev"
fi
if [ -n "$missing" ]; then
    echo "缺少依赖包：$missing"
    echo "请先执行：sudo apt install$missing"
    exit 1
fi

# ---- 2) dde-dock 接口头 ----
echo "==> 检查 dde-dock 插件接口头 ..."
DDE_HDR=""
for d in /usr/include/dde-dock "$DDE_DOCK_INCLUDE_DIR"; do
    if [ -n "$d" ] && [ -f "$d/pluginsiteminterface.h" ]; then
        DDE_HDR="$d"
        break
    fi
done
if [ -z "$DDE_HDR" ]; then
    echo "找不到 pluginsiteminterface.h，尝试自动安装 dde-dock-dev ..."
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get install -y dde-dock-dev || true
    fi
    [ -f /usr/include/dde-dock/pluginsiteminterface.h ] && DDE_HDR=/usr/include/dde-dock
fi
if [ -z "$DDE_HDR" ]; then
    echo "!! 仍找不到接口头。请手动安装：sudo apt install dde-dock-dev" >&2
    echo "   或者去 https://github.com/linuxdeepin/dde-dock 取 interfaces/ 下的三个头文件" >&2
    exit 1
fi
echo "   使用：$DDE_HDR"

# ---- 3) 编译 ----
echo "==> 配置 CMake ..."
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DDDE_DOCK_INCLUDE_DIR="$DDE_HDR"

echo "==> 编译 ..."
cmake --build . -j"$(nproc)"

SO="$SCRIPT_DIR/build/libcartoon-eye.so"
[ -f "$SO" ] || { echo "!! 编译失败，未生成 $SO" >&2; exit 1; }

echo ""
echo "==> 完成：$SO"
readelf -d "$SO" 2>/dev/null | grep NEEDED | sed 's/^/    /' || true
echo ""
echo "下一步二选一："
echo "  A) 打包（推荐，会自动带上 V25 那套）"
echo "       cd ../dist && bash make-dual-deb.sh 1.9.7"
echo "  B) 单测这一套（先建立软链再重启任务栏）"
echo "       sudo install -Dm755 $SO /usr/lib/dde-dock/plugins/libcartoon-eye.so"
echo "       killall dde-dock; nohup dde-dock >/dev/null 2>&1 &"

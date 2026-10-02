#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 交叉编译 deepin V20 (dde-dock / Qt5) 版插件，产物直接放到 prebuilt/
# 供 dist/make-dual-deb.sh 打包使用。
#
# 前提：先跑过 setup-sysroot.sh 准备好 buster Qt 5.11.3 sysroot。
#
# 用法：
#   bash cross-build.sh
#   V20_SYSROOT=/path/to/sysroot bash cross-build.sh
#
# 产物：linux/v20/prebuilt/libcartoon-eye.so
#
# 为什么必须用 sysroot 而不是本机 Qt：
#   deepin 20.0~20.8 是 Qt 5.11.3 / glibc 2.28。用高版本工具链编出来的 .so
#   会引用 GLIBC_2.38、Qt_6.x 之类的符号版本，V20 加载时直接失败。
#   注意 std::fmod 就是个坑：在本机 glibc 2.38 上会绑定 fmod@GLIBC_2.38，
#   所以代码里用自写的 modPos() 代替（见 eyeartist.cpp）。

set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSROOT="${V20_SYSROOT:-$HOME/.cache/dde-shell-eye-plugin/v20-sysroot}"
OUT="$ROOT/prebuilt"
OBJ="$ROOT/build-cross"

QINC="$SYSROOT/usr/include/x86_64-linux-gnu/qt5"
# 有的发行版把 Qt 头放在这个位置
[ -d "$QINC/QtWidgets" ] || QINC="$SYSROOT/usr/include/qt5"
DDEINC="$SYSROOT/usr/include/dde-dock"
QLIB="$SYSROOT/usr/lib/x86_64-linux-gnu"
MOC="${V20_MOC:-$(find "$SYSROOT" -type f -name moc 2>/dev/null | head -1)}"

die() { echo "!! $*" >&2; exit 1; }

[ -d "$QINC/QtWidgets" ] || die "找不到 Qt5 头文件（$QINC）。
   请先执行：bash $ROOT/setup-sysroot.sh"
[ -n "$MOC" ] && [ -x "$MOC" ] || die "找不到 moc。请先执行：bash $ROOT/setup-sysroot.sh"
[ -f "$DDEINC/pluginsiteminterface.h" ] || die "找不到 dde-dock 接口头（$DDEINC）。
   请先执行：bash $ROOT/setup-sysroot.sh"

echo "==> sysroot : $SYSROOT"
echo "==> Qt5 头  : $QINC"
echo "==> dde 头  : $DDEINC"
echo "==> moc     : $MOC"

INCS=(-I"$QINC" -I"$QINC/QtCore" -I"$QINC/QtGui" -I"$QINC/QtWidgets" -I"$QINC/QtNetwork"
      -I"$DDEINC" -I"$ROOT" -I"$ROOT/package")

rm -rf "$OBJ"
mkdir -p "$OBJ" "$OUT"
cd "$OBJ"

echo "==> moc ..."
for h in eyeplugin.h eyewidget.h eyemenu.h eyepopup.h; do
    echo "    $h"
    "$MOC" "${INCS[@]}" "$ROOT/$h" -o "moc_${h%.h}.cpp"
done

echo "==> 编译 + 链接 ..."
g++ -fPIC -shared -std=c++14 -O2 \
    -o "$OUT/libcartoon-eye.so" \
    "$ROOT/eyeplugin.cpp" "$ROOT/eyeartist.cpp" "$ROOT/eyewidget.cpp" \
    "$ROOT/eyemenu.cpp" "$ROOT/eyepopup.cpp" \
    moc_eyeplugin.cpp moc_eyewidget.cpp moc_eyemenu.cpp moc_eyepopup.cpp \
    "${INCS[@]}" \
    -L"$QLIB" -Wl,-rpath-link,"$QLIB" \
    -lQt5Core -lQt5Gui -lQt5Widgets -lQt5Network

SO="$OUT/libcartoon-eye.so"

echo
echo "==> ABI 自检（必须能被 deepin 20 / glibc 2.28 / Qt 5.11 加载）"
echo "  NEEDED:"
readelf -d "$SO" | grep NEEDED | sed 's/^/    /'

MAX_GLIBC=$(readelf --version-info "$SO" | grep -oE 'GLIBC_[0-9.]+' | sort -uV | tail -1)
MAX_QT=$(readelf --version-info "$SO" | grep -oE 'Qt_[0-9.]+' | sort -uV | tail -1)
echo "  最高 GLIBC 符号版本: $MAX_GLIBC"
echo "  最高 Qt 符号版本  : $MAX_QT"

grep -qa 'com\.deepin\.dock\.PluginsItemInterface' "$SO" \
    || die "$SO 里找不到 IID com.deepin.dock.PluginsItemInterface"

# glibc 2.28 是 deepin 20 的版本；出现更高版本号说明绑到了本机 glibc
case "$MAX_GLIBC" in
    GLIBC_2.[0-9]|GLIBC_2.1[0-9]|GLIBC_2.2[0-8]) : ;;
    *) echo "  !! 警告：$SO 需要 $MAX_GLIBC，deepin 20 (glibc 2.28) 可能加载失败" >&2 ;;
esac

echo
echo "==> 完成：$SO"
ls -la "$SO"

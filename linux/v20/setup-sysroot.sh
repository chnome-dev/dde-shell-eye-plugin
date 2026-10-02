#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 为非 deepin V20 的机器准备一个「V20 编译环境」（sysroot）。
#
# 为什么要 sysroot：
#   V20 版插件必须能被 deepin 20 加载，而 deepin 20.0~20.8 是 Qt 5.11.3 +
#   glibc 2.28。如果直接用本机（比如 deepin 25，Qt6 / glibc 2.38）的 Qt 编，
#   产出的 .so 会带上 GLIBC_2.38、Qt_6 之类的符号版本，V20 根本加载不了。
#   所以这里从 archive.debian.org 拉 Debian buster 的 Qt 5.11.3 开发包，
#   解包成一个独立目录，再用它做交叉编译。
#
# 用法：
#   bash setup-sysroot.sh                 # 装到默认位置
#   V20_SYSROOT=/path/to/sysroot bash setup-sysroot.sh
#
# 默认位置：$HOME/.cache/dde-shell-eye-plugin/v20-sysroot
# 幂等：已下载的包不会重复下载。

set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSROOT="${V20_SYSROOT:-$HOME/.cache/dde-shell-eye-plugin/v20-sysroot}"
PKGS="$SYSROOT/.pkgs"

DEBIAN_BASE="https://archive.debian.org/debian/pool/main"
DEEPIN_BASE="https://community-packages.deepin.com/deepin/pool/main"

# buster 里 Qt 5.11.3 的修订号，按新旧顺序试。
# 归档里的文件名是固定不变的，所以直接写死精确 URL——不走目录列表
# （archive.debian.org 的目录列表接口偶尔会抽风返回 404，实测过）。
QT_VERSIONS="5.11.3+dfsg1-1+deb10u5
5.11.3+dfsg1-1+deb10u4
5.11.3+dfsg1-1+deb10u3
5.11.3+dfsg1-1+deb10u2
5.11.3+dfsg1-1+deb10u1
5.11.3+dfsg1-1"

mkdir -p "$PKGS" "$SYSROOT"

# 下载并解包；失败返回 1（不退出）
fetch_deb()
{
    local url="$1" f
    f="$PKGS/$(basename "$url")"
    if [ ! -s "$f" ]; then
        echo "  --> $(basename "$f")"
        if ! curl -fsSL --retry 3 --retry-delay 1 "$url" -o "$f.part"; then
            rm -f "$f.part"
            return 1
        fi
        mv "$f.part" "$f"
    fi
    dpkg-deb -x "$f" "$SYSROOT"
}

# 按 QT_VERSIONS 依次尝试
fetch_qt()
{
    local name="$1" ver
    for ver in $QT_VERSIONS; do
        if fetch_deb "$DEBIAN_BASE/q/qtbase-opensource-src/${name}_${ver}_amd64.deb" 2>/dev/null; then
            return 0
        fi
    done
    echo "  !! $name 下载失败" >&2
    return 1
}

echo "==> sysroot: $SYSROOT"

echo "==> 1/3 拉取 Debian buster 的 Qt 5.11.3（Core/Gui/Widgets/Network）"
for p in qtbase5-dev qtbase5-dev-tools \
         libqt5core5a libqt5gui5 libqt5widgets5 libqt5network5 \
         libqt5dbus5 libqt5opengl5 libqt5printsupport5; do
    fetch_qt "$p" || true
done

# Qt5Core 的运行期依赖，sysroot 里也要有，否则交叉链接会报 undefined reference
for u in \
    "$DEBIAN_BASE/i/icu/libicu63_63.1-6+deb10u3_amd64.deb" \
    "$DEBIAN_BASE/d/double-conversion/libdouble-conversion1_3.1.0-3_amd64.deb"; do
    fetch_deb "$u" || echo "  !! 拉取失败：$u" >&2
done

echo "==> 2/3 拉取 dde-dock 插件接口头（dde-dock-dev）"
# 用官方仓库里最新的 dde-dock-dev：它的 PluginsItemInterface 虚函数表是
# 旧版本的「超集」（新版只在末尾追加了 pluginSizePolicy），
# 所以用它编出来的插件在 deepin 20 的老 dde-dock 上同样能用。
for v in 5.5.86.1-1 5.5.78-1 5.5.73-1 5.5.53-1 5.5.12-1; do
    fetch_deb "$DEEPIN_BASE/d/dde-dock/dde-dock-dev_${v}_amd64.deb" 2>/dev/null && break
done

echo "==> 3/3 检查"
QINC="$SYSROOT/usr/include/x86_64-linux-gnu/qt5"
MOC=$(find "$SYSROOT" -type f -name moc 2>/dev/null | head -1)
DDEINC="$SYSROOT/usr/include/dde-dock"

ok=1
[ -d "$QINC/QtWidgets" ] || { echo "  !! 缺少 Qt5 头文件 (${QINC}/QtWidgets)" >&2; ok=0; }
[ -n "$MOC" ]            || { echo "  !! 缺少 moc" >&2; ok=0; }
[ -f "$DDEINC/pluginsiteminterface.h" ] || { echo "  !! 缺少 pluginsiteminterface.h" >&2; ok=0; }
[ "$ok" = 1 ] || exit 1

echo "  Qt 头文件  : $QINC"
echo "  moc        : $MOC"
echo "  dde-dock 头: $DDEINC"
echo
echo "==> 完成。接下来执行："
echo "    V20_SYSROOT=$SYSROOT bash $ROOT/cross-build.sh"

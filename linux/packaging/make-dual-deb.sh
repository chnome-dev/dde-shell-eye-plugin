#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 组装「单包双架构」deb：一个 deb 内同时含 V25(dde-shell/Qt6) 与
# V20(dde-dock/Qt5) 两套插件，由 postinst 按系统自动启用匹配的那一套。
#
# 用法：
#   bash make-dual-deb.sh [版本号] [V20的libcartoon-eye.so路径]
#
# 默认版本号取自 v20/CMakeLists.txt 里的 project(... VERSION x.y.z)。
#
# V20 的 .so 按顺序在这些位置找（也可以用第 2 个参数显式指定）：
#   1) 命令行参数
#   2) linux/v20/build/libcartoon-eye.so       （在 V20 机器上跑 build-v20.sh 的产物）
#   3) linux/v20/prebuilt/libcartoon-eye.so    （仓库内自带的交叉编译产物）
#
# 找不到 V20 的 .so 会直接报错退出——1.9.7 就是吃了这个亏：
# 包里 usr/lib/dde-dock/plugins/ 是个空目录，V20 装上后什么也没有。

set -e

PKG_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$(cd "$PKG_ROOT/.." && pwd)"

# 版本号默认从 v20/CMakeLists.txt 读
if [ -n "${1:-}" ]; then
    VER="$1"
else
    VER=$(sed -n 's/^project(.*VERSION[[:space:]]\+\([0-9][0-9.]*\).*/\1/p' \
          "$LINUX_DIR/v20/CMakeLists.txt" | head -1)
    [ -n "$VER" ] || VER="1.9.7"
fi
V20_SO="${2:-}"

V25_SO_SRC="$LINUX_DIR/build/plugins/org.deepin.ds.dock.eye.so"
[ -f "$V25_SO_SRC" ] || V25_SO_SRC="$LINUX_DIR/build/org.deepin.ds.dock.eye.so"

if [ -z "$V20_SO" ]; then
    for cand in "$LINUX_DIR/v20/build/libcartoon-eye.so" \
                "$LINUX_DIR/v20/prebuilt/libcartoon-eye.so"; do
        if [ -f "$cand" ]; then V20_SO="$cand"; break; fi
    done
fi

PKG_DIR="$LINUX_DIR/dist/dde-shell-eye-plugin_${VER}_amd64"
DEB="$LINUX_DIR/dist/dde-shell-eye-plugin_${VER}_amd64.deb"

echo "==> 版本: $VER"

# ---- 0) 前置校验：两个 .so 都必须存在，而且必须是「对的那一套」 ----
if [ ! -f "$V25_SO_SRC" ]; then
    echo "!! 未找到 V25 (dde-shell/Qt6) 的 .so：$V25_SO_SRC"
    echo "   请先在 linux/ 目录执行：cmake -B build && cmake --build build -j\$(nproc)"
    exit 1
fi
if [ -z "$V20_SO" ] || [ ! -f "$V20_SO" ]; then
    echo "!! 未找到 V20 (dde-dock/Qt5) 的 .so。查找过："
    echo "     $LINUX_DIR/v20/build/libcartoon-eye.so"
    echo "     $LINUX_DIR/v20/prebuilt/libcartoon-eye.so"
    echo "   生成方式二选一："
    echo "     * 在 deepin V20 机器上：  cd linux/v20 && bash build-v20.sh"
    echo "     * 在任何机器上交叉编译：  bash linux/v20/setup-sysroot.sh && bash linux/v20/cross-build.sh"
    echo "   （不要拿 V25 的 .so 顶替——那是 Qt6/dde-shell 插件，V20 加载不了）"
    exit 1
fi

# 校验 V20 .so 的 IID 与依赖，避免误把 V25 的 .so 打进来
if ! grep -qa "com.deepin.dock.PluginsItemInterface" "$V20_SO"; then
    echo "!! $V20_SO 不是 dde-dock (V1) 插件：找不到 IID com.deepin.dock.PluginsItemInterface"
    exit 1
fi
if readelf -d "$V20_SO" 2>/dev/null | grep -q "libdde-shell"; then
    echo "!! $V20_SO 链接了 libdde-shell（那是 V25 的插件），不能在 V20 上用"
    exit 1
fi
echo "==> V20 插件: $V20_SO"
echo "==> V25 插件: $V25_SO_SRC"

# ---- 1) 准备包目录 ----
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/lib/x86_64-linux-gnu/dde-shell"
mkdir -p "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/data"
mkdir -p "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/icons"
mkdir -p "$PKG_DIR/usr/share/dde-dock/cartoon-eye/data"
# V20 的 .so 先放在中立目录，由 postinst 软链到 /usr/lib/dde-dock/plugins/
mkdir -p "$PKG_DIR/usr/lib/dde-shell-eye-plugin/v20"
mkdir -p "$PKG_DIR/usr/share/doc/dde-shell-eye-plugin"

# ---- 2) V25 部分（dde-shell） ----
echo "==> 复制 V25 (dde-shell) 插件 ..."
cp "$V25_SO_SRC" "$PKG_DIR/usr/lib/x86_64-linux-gnu/dde-shell/org.deepin.ds.dock.eye.so"
cp "$LINUX_DIR/package/main.qml"      "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/main.qml"
cp "$LINUX_DIR/package/metadata.json" "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/metadata.json"
cp "$LINUX_DIR/package/icons/eye.svg" "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/icons/eye.svg"
cp "$LINUX_DIR/package/data/"*.json   "$PKG_DIR/usr/share/dde-shell/org.deepin.ds.dock.eye/data/"

# ---- 3) V20 部分（dde-dock） ----
echo "==> 复制 V20 (dde-dock) 插件 ..."
cp "$V20_SO" "$PKG_DIR/usr/lib/dde-shell-eye-plugin/v20/libcartoon-eye.so"
# V20 与 V25 共用同一批学习数据
cp "$LINUX_DIR/package/data/"*.json     "$PKG_DIR/usr/share/dde-dock/cartoon-eye/data/"
cp "$LINUX_DIR/v20/package/plugin.json" "$PKG_DIR/usr/share/dde-dock/cartoon-eye/plugin.json"

# ---- 4) 维护脚本 ----
echo "==> 写入 DEBIAN 控制文件 ..."
cp "$PKG_ROOT/control"         "$PKG_DIR/DEBIAN/control"
cp "$PKG_ROOT/postinst"        "$PKG_DIR/DEBIAN/postinst"
cp "$PKG_ROOT/prerm"           "$PKG_DIR/DEBIAN/prerm"
cp "$PKG_ROOT/restart-dock.sh" "$PKG_DIR/usr/lib/dde-shell-eye-plugin/restart-dock.sh"
chmod 755 "$PKG_DIR/DEBIAN/postinst" "$PKG_DIR/DEBIAN/prerm" \
          "$PKG_DIR/usr/lib/dde-shell-eye-plugin/restart-dock.sh"

sed -i "s/^Version: .*/Version: $VER/" "$PKG_DIR/DEBIAN/control"

cat > "$PKG_DIR/usr/share/doc/dde-shell-eye-plugin/copyright" <<'EOF'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: dde-shell-eye-plugin
Source: https://github.com/chnome-dev/dde-shell-eye-plugin

Files: *
Copyright: 2026 CHNOME
License: GPL-3.0-or-later
EOF

# ---- 5) md5sums + Installed-Size ----
echo "==> 计算 md5sums / Installed-Size ..."
( cd "$PKG_DIR" && find . -type f ! -path "./DEBIAN/*" -exec md5sum {} \; > DEBIAN/md5sums )
SIZE_KB=$(du -sk --exclude=DEBIAN "$PKG_DIR" | cut -f1)
sed -i "s/^Installed-Size: .*/Installed-Size: $SIZE_KB/" "$PKG_DIR/DEBIAN/control"

# ---- 6) 打包 ----
echo "==> 生成 deb ..."
cd "$LINUX_DIR/dist"
rm -f "$DEB"
# 尽量把包内文件属主固定成 root:root（老 dpkg 没有这个选项就跳过）
ROOT_OWNER_FLAG=""
if dpkg-deb --help 2>&1 | grep -q -- "--root-owner-group"; then
    ROOT_OWNER_FLAG="--root-owner-group"
fi
dpkg-deb $ROOT_OWNER_FLAG --build "dde-shell-eye-plugin_${VER}_amd64" "dde-shell-eye-plugin_${VER}_amd64.deb"

echo ""
echo "==> 完成：$DEB"
echo ""
echo "==> 包内关键文件自检："
dpkg-deb -c "$DEB" | grep -E "dde-shell/org\.deepin|dde-shell-eye-plugin/v20|restart-dock" || true
echo ""
echo "==> Depends / Installed-Size："
dpkg-deb -f "$DEB" Depends Recommends Installed-Size
echo ""
echo "==> V20 那套 .so 的 ABI 自检（必须只依赖 Qt5 / GLIBC<=2.28）："
# 不要用 /tmp —— 某些 deepin 环境 /tmp 是只有 10MB 的 tmpfs，解包会失败
VERIFY_DIR="$LINUX_DIR/dist/.verify.$$"
rm -rf "$VERIFY_DIR"
mkdir -p "$VERIFY_DIR"
dpkg-deb -x "$DEB" "$VERIFY_DIR"
SO="$VERIFY_DIR/usr/lib/dde-shell-eye-plugin/v20/libcartoon-eye.so"
if [ -f "$SO" ]; then
    readelf -d "$SO" | grep NEEDED | sed 's/^/    /'
    echo "    最高 GLIBC 符号版本: $(readelf --version-info "$SO" | grep -oE 'GLIBC_[0-9.]+' | sort -uV | tail -1)"
    echo "    最高 Qt 符号版本  : $(readelf --version-info "$SO" | grep -oE 'Qt_[0-9.]+' | sort -uV | tail -1)"
else
    echo "    !! 包里没有 V20 的 .so"
    rm -rf "$VERIFY_DIR"
    exit 1
fi
rm -rf "$VERIFY_DIR"

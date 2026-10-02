#!/bin/bash
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# postinst / prerm 的回归测试：验证「按系统选对插件」这件事真的对。
#
# 做法：把脚本里的绝对路径重定向到一个临时的伪造根目录，再在受控 PATH 下实跑，
# 覆盖 deepin 25 / deepin 20 / 类 deepin 23 / 非 DDE / 卸载 / 升级 等场景。
#
# 用法：bash test-deploy.sh
#
# 为什么值得留着：这套判定的依据全是从真实系统里踩出来的坑——
#   * deepin 25 的 libdde-shell soname 是 .so.1（库版本 2.0.x），不是 .so.2
#   * /usr/lib/dde-dock/plugins 在 deepin 25 上也存在，不能拿来先判 V20
#   * 把 Qt5 插件直接装进 /usr/lib/dde-dock/plugins 会被 V25 的托盘加载器扫到
# 改动维护脚本后跑一遍，能立刻发现是不是又把顺序或判据写错了。

set -u

PKG_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$(cd "$PKG_ROOT/.." && pwd)"
V20SO="$LINUX_DIR/v20/prebuilt/libcartoon-eye.so"

[ -f "$V20SO" ] || { echo "缺少 $V20SO（先跑 linux/v20/cross-build.sh）" >&2; exit 1; }

FAKE="$(mktemp -d "${TMPDIR:-/tmp}/dse-deploy-test.XXXXXX")"
trap 'rm -rf "$FAKE"' EXIT

L="$FAKE/usr/lib/dde-dock/plugins/libcartoon-eye.so"
V20DIR="$FAKE/usr/lib/dde-dock/plugins"
LIBDIR="$FAKE/usr/lib/x86_64-linux-gnu"

mkdir -p "$FAKE/usr/bin" "$V20DIR" "$LIBDIR" \
         "$FAKE/usr/lib/dde-shell-eye-plugin/v20" "$FAKE/etc" "$FAKE/fakebin"

for s in postinst prerm restart-dock.sh; do
    sed "s#/usr/#$FAKE/usr/#g; \
         s#/etc/deepin-version#$FAKE/etc/deepin-version#g; \
         s#/run/user/#$FAKE/run/user/#g" "$PKG_ROOT/$s" > "$FAKE/$s"
done
chmod +x "$FAKE/postinst" "$FAKE/prerm"
cp "$V20SO" "$FAKE/usr/lib/dde-shell-eye-plugin/v20/libcartoon-eye.so"
cp "$PKG_ROOT/restart-dock.sh" "$FAKE/usr/lib/dde-shell-eye-plugin/restart-dock.sh"
chmod 755 "$FAKE/usr/lib/dde-shell-eye-plugin/restart-dock.sh"

# 受控 PATH：只放必需工具（显式指向 /bin 或 /usr/bin 下的真二进制），
# 绝不暴露本机真实的 dde-shell / dde-dock。
for c in sed grep find id su mkdir kill sleep chown chmod ps awk tr head cat printf \
         mktemp cut du dirname basename readlink touch pgrep sh bash rm ln; do
    real=""
    for d in /bin /usr/bin /usr/sbin /sbin; do
        if [ -x "$d/$c" ]; then real="$d/$c"; break; fi
    done
    [ -n "$real" ] || continue
    printf '#!/bin/sh\nexec %s "$@"\n' "$real" > "$FAKE/fakebin/$c"
    chmod +x "$FAKE/fakebin/$c"
done

# 用 env -i 清空环境：某些沙箱/终端环境会把 rm 变成「导出函数」（走回收站），
# 那会盖掉 PATH 里的真 rm，让测试出现假阴性。
run() { env -i PATH="$FAKE/fakebin" HOME=/root SUDO_USER= /bin/bash "$@"; }

pass=0; fail=0
chk() { if [ "$2" = 1 ]; then echo "  [OK]   $1"; pass=$((pass+1));
        else echo "  [FAIL] $1"; fail=$((fail+1)); fi; }
link_exists() { [ -L "$L" ] && echo 1 || echo 0; }

echo "=== A: deepin 25（有 /usr/bin/dde-shell，且 /usr/lib/dde-dock/plugins 目录也存在）==="
touch "$FAKE/usr/bin/dde-shell" "$LIBDIR/libdde-shell.so.2.0.52"
ln -sfn libdde-shell.so.2.0.52 "$LIBDIR/libdde-shell.so.1"
out=$(run "$FAKE/postinst"); echo "$out" | sed 's/^/       /'
chk "判为 v25" "$(echo "$out" | grep -q '平台 = v25' && echo 1 || echo 0)"
chk "不建立 V20 软链" "$([ "$(link_exists)" = 0 ] && echo 1 || echo 0)"

echo "=== B: deepin 20（无 dde-shell，有 dde-dock + /usr/lib/dde-dock/plugins）==="
rm -f "$FAKE/usr/bin/dde-shell" "$LIBDIR/libdde-shell.so.1" \
      "$LIBDIR/libdde-shell.so.2.0.52" "$L"
touch "$FAKE/usr/bin/dde-dock"
out=$(run "$FAKE/postinst"); echo "$out" | sed 's/^/       /'
chk "判为 v20" "$(echo "$out" | grep -q '平台 = v20' && echo 1 || echo 0)"
chk "软链指向包内暂存的 V20 .so" \
    "$([ "$(readlink "$L" 2>/dev/null)" = "$FAKE/usr/lib/dde-shell-eye-plugin/v20/libcartoon-eye.so" ] && echo 1 || echo 0)"

echo "=== C: 类 deepin 23（无 dde-shell 命令，但有 libdde-shell.so.1；dde-dock/plugins 目录也在）==="
rm -f "$FAKE/usr/bin/dde-dock" "$L"
touch "$LIBDIR/libdde-shell.so.1"
out=$(run "$FAKE/postinst"); echo "$out" | sed 's/^/       /'
chk "仍判为 v25（不被 dde-dock/plugins 目录带偏）" \
    "$(echo "$out" | grep -q '平台 = v25' && echo 1 || echo 0)"
chk "不建立 V20 软链" "$([ "$(link_exists)" = 0 ] && echo 1 || echo 0)"

echo "=== D: V20 上 prerm remove 应摘掉软链 ==="
rm -f "$LIBDIR/libdde-shell.so.1"
touch "$FAKE/usr/bin/dde-dock"
run "$FAKE/postinst" >/dev/null
chk "postinst 后软链存在" "$(link_exists)"
run "$FAKE/prerm" remove >/dev/null
chk "prerm remove 后软链已清理" "$([ "$(link_exists)" = 0 ] && echo 1 || echo 0)"

echo "=== E: prerm upgrade 也应清掉软链（随后由 postinst 重建）==="
run "$FAKE/postinst" >/dev/null
run "$FAKE/prerm" upgrade 2.0.0 >/dev/null
chk "prerm upgrade 后软链已清理" "$([ "$(link_exists)" = 0 ] && echo 1 || echo 0)"

echo "=== F: V25 上 prerm remove 不应报错 ==="
rm -f "$FAKE/usr/bin/dde-dock" "$L"
touch "$FAKE/usr/bin/dde-shell"
run "$FAKE/prerm" remove >/dev/null
chk "退出码为 0" "$([ $? = 0 ] && echo 1 || echo 0)"

echo "=== G: 非 DDE 系统（无 dde-shell、无 dde-dock、也无插件目录）==="
rm -f "$FAKE/usr/bin/dde-shell" "$L"
rmdir "$V20DIR" 2>/dev/null
out=$(run "$FAKE/postinst"); echo "$out" | sed 's/^/       /'
chk "判为 unknown" "$(echo "$out" | grep -q '平台 = unknown' && echo 1 || echo 0)"
chk "不建立 V20 软链" "$([ "$(link_exists)" = 0 ] && echo 1 || echo 0)"

echo
echo "结果：通过 $pass 项，失败 $fail 项"
[ "$fail" = 0 ]

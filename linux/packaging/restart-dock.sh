#!/bin/sh
# SPDX-FileCopyrightText: 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 重启 DDE 任务栏，让插件立刻生效/消失。
# postinst 与 prerm 都会调用它，参数为平台名（v20 / v25 / unknown）。
#
# 为什么 V20 需要这么麻烦：
#   dde-dock 只在启动时扫一遍 /usr/lib/dde-dock/plugins（pluginloader 里没有
#   QFileSystemWatcher），也没有提供「重新加载插件」的 D-Bus 方法
#   （com.deepin.dde.Dock 只暴露 geometry）。所以只能让新实例接管。
#   kill 掉旧实例后先等会话管理器自动拉起；等不到就自己拉起——拉起时必须用
#   从旧进程 /proc/PID/environ 里抓到的会话环境，否则 DISPLAY /
#   DBUS_SESSION_BUS_ADDRESS 不对，任务栏会真的消失。
#
# 本脚本永不返回非 0，也刻意不用 `set -e`。

PLATFORM="${1:-unknown}"

has_proc()
{
    if command -v pgrep >/dev/null 2>&1; then
        pgrep -u "$1" -x dde-dock >/dev/null 2>&1
        return $?
    fi
    ps -u "$1" -o comm= 2>/dev/null | grep -qx dde-dock
}

session_user()
{
    _su="${SUDO_USER:-}"
    [ -n "$_su" ] || return 1
    _uid=$(id -u "$_su" 2>/dev/null) || return 1
    [ -n "$_uid" ] || return 1
    [ -d "/run/user/$_uid" ] || return 1
    echo "$_su $_uid"
}

restart_v20()
{
    set -- $(session_user) || return 0
    _user="$1"
    _uid="$2"

    if command -v pgrep >/dev/null 2>&1; then
        _pid=$(pgrep -u "$_uid" -x dde-dock 2>/dev/null | head -1)
    else
        _pid=$(ps -u "$_uid" -o pid=,comm= 2>/dev/null | awk '$2 == "dde-dock" { print $1; exit }')
    fi
    [ -n "$_pid" ] || return 0

    # 1) 抓会话环境
    _envfile=$(mktemp /tmp/dde-dock-env.XXXXXX 2>/dev/null) || _envfile=""
    if [ -n "$_envfile" ] && [ -r "/proc/$_pid/environ" ]; then
        tr '\0' '\n' < "/proc/$_pid/environ" > "$_envfile" 2>/dev/null || : > "$_envfile"
    fi

    # 2) 杀掉旧实例，看会话管理器会不会自己把它拉起来
    kill "$_pid" 2>/dev/null || true
    _n=0
    while [ "$_n" -lt 6 ]; do
        sleep 1
        if has_proc "$_uid"; then
            [ -n "$_envfile" ] && rm -f "$_envfile"
            echo "dde-shell-eye-plugin: 任务栏已自动重启"
            return 0
        fi
        _n=$((_n + 1))
    done

    # 3) 没被拉起，就用抓到的环境自己拉
    if [ -z "$_envfile" ] || [ ! -s "$_envfile" ]; then
        [ -n "$_envfile" ] && rm -f "$_envfile"
        echo "dde-shell-eye-plugin: 未能读取任务栏会话环境，跳过自动重启。"
        echo "  请注销后重新登录，或手动执行：killall dde-dock; nohup dde-dock >/dev/null 2>&1 &"
        return 0
    fi

    _runner=$(mktemp /tmp/dde-dock-run.XXXXXX 2>/dev/null) || _runner=""
    if [ -z "$_runner" ]; then
        rm -f "$_envfile"
        echo "dde-shell-eye-plugin: 请注销后重新登录，或手动执行：killall dde-dock; nohup dde-dock >/dev/null 2>&1 &"
        return 0
    fi

    {
        echo '#!/bin/sh'
        grep -E '^(DISPLAY|WAYLAND_DISPLAY|XAUTHORITY|DBUS_SESSION_BUS_ADDRESS|XDG_RUNTIME_DIR|XDG_SESSION_TYPE|XDG_SESSION_ID|XDG_CURRENT_DESKTOP|XDG_SESSION_DESKTOP|XDG_DATA_DIRS|XDG_CONFIG_DIRS|DESKTOP_SESSION|QT_QPA_PLATFORM|GDK_BACKEND|LANG|LC_ALL|LC_CTYPE|PATH|HOME|USER|LOGNAME|SHELL)=' "$_envfile" \
            | sed "s/^\([A-Za-z_][A-Za-z0-9_]*\)=\(.*\)$/export \1='\2'/"
        echo "[ -n \"\${XDG_RUNTIME_DIR:-}\" ] || export XDG_RUNTIME_DIR='/run/user/$_uid'"
        echo 'exec dde-dock >/dev/null 2>&1'
    } > "$_runner" 2>/dev/null
    rm -f "$_envfile"
    chmod 700 "$_runner" 2>/dev/null || true
    chown "$_uid" "$_runner" 2>/dev/null || true

    su -s /bin/sh "$_user" -c "$_runner" >/dev/null 2>&1 &
    sleep 2
    rm -f "$_runner"

    if has_proc "$_uid"; then
        echo "dde-shell-eye-plugin: 已重启任务栏"
    else
        echo "dde-shell-eye-plugin: 自动重启任务栏失败。"
        echo "  请注销后重新登录，或手动执行：killall dde-dock; nohup dde-dock >/dev/null 2>&1 &"
    fi
}

restart_v25()
{
    _info=$(session_user) || return 0
    [ -n "$_info" ] || return 0
    # shellcheck disable=SC2086
    set -- $_info
    _user="$1"
    _uid="$2"
    su -s /bin/sh "$_user" -c \
        "XDG_RUNTIME_DIR=/run/user/$_uid systemctl --user restart dde-shell@DDE" \
        >/dev/null 2>&1 || true
    echo "dde-shell-eye-plugin: 已请求重启 dde-shell"
}

case "$PLATFORM" in
v20) restart_v20 ;;
v25) restart_v25 ;;
*)   : ;;
esac

exit 0

# Linux 版打包说明（deepin V20 + V25 双兼容）

本文讲清楚三件事：

1. 1.9.6 在 deepin V20 上为什么装不上（根因）
2. V20 与 V25 的任务栏插件体系差在哪
3. 怎么从源码构建、测试、打出**一个同时兼容两代 deepin 的 deb**

---

## 1. 根因：1.9.6 为什么在 deepin V20 装不上

安装时 dpkg 报的错是：

```
dde-shell-eye-plugin : 依赖: dde-shell (>= 2.0) 但无法安装它
                       依赖: libdde-shell (>= 2.0) 但无法安装它
                       依赖: libdde-shell-dock (>= 2.0) 但无法安装它
```

这不是"依赖没装"，而是**包本身是给另一套任务栏体系构建的**。有三个层次的问题：

### 1.1 依赖项写死了 V25 的库

1.9.6 的 `control` 由 V25 的构建脚本生成，`Depends` 直接写成了：

```
Depends: dde-shell (>= 2.0), libdde-shell (>= 2.0), libdde-shell-dock (>= 2.0)
```

而 deepin V20 里**根本没有** `dde-shell` 这个东西（V20 的任务栏叫 `dde-dock`）。于是 dpkg 在依赖解析阶段就直接拒绝安装。

### 1.2 插件二进制是 Qt6/dde-shell 专用的

就算用 `dpkg -i --force-depends` 强装，也仍然用不了：

- 包内 `.so` 装在 `/usr/lib/x86_64-linux-gnu/dde-shell/`，而 V20 的 dde-dock 只扫描 `/usr/lib/dde-dock/plugins/`；
- 该 `.so` 动态链接 `libdde-shell.so.1`，在 V20 上这个库不存在，`dlopen` 必失败；
- 它是 Qt6 编译的，V20 的 dde-dock 是 Qt5 进程，ABI 不兼容。

### 1.3 （历史遗留）`v20/` 源码用错了接口版本

仓库里当时那份 `v20/` 源码用的是 `PluginsItemInterfaceV2`、`plugin.json` 里 `api` 写 `2.0`。这两样都是 **deepin 23+ / dde-tray-loader** 的 API：

- deepin V20 的 dde-dock 5.x 只有 **V1** 接口 `PluginsItemInterface`，IID 是 `com.deepin.dock.PluginsItemInterface`；
- dde-dock 加载插件时有一道 **api 白名单**（`frame/util/abstractpluginscontroller.cpp`）：

  ```cpp
  const QStringList CompatiblePluginApiList { "1.1.1", "1.2", "1.2.1", DOCK_PLUGIN_API_VERSION };
  ```

  `api: "2.0"` 不在白名单里，插件会被静默拒载。

**结论**：不是"补个依赖"能修的，必须为 V20 真正编一套 dde-dock 插件。

---

## 2. V20 与 V25 的差异

| | **deepin V25 / V23** | **deepin V20 / V15** |
|---|---|---|
| 任务栏框架 | `dde-shell` 2.x | `dde-dock` 5.x |
| Qt 版本 | Qt 6 | Qt 5（20.0~20.8 是 **5.11.3**，20.9 起 5.15） |
| glibc | 2.36+ | **2.28** |
| 插件目录 | `/usr/lib/x86_64-linux-gnu/dde-shell/` | `/usr/lib/dde-dock/plugins/` |
| 插件接口 | dde-shell Applet（QML + C++ applet） | `PluginsItemInterface`（**V1**） |
| IID | `org.deepin.ds.dock.eye` | `com.deepin.dock.PluginsItemInterface` |
| 元数据文件 | `metadata.json` | `plugin.json`（带 api 白名单校验） |
| UI 技术 | Qt Quick / QML | QWidget / QPainter |
| 数据根目录 | `/usr/share/dde-shell/<id>/` | `/usr/share/dde-dock/<name>/` |

两套体系 ABI 完全不兼容，没有任何"一个 `.so` 通吃"的办法。

---

## 3. 方案：单 deb，内含两套实现

```
dde-shell-eye-plugin_<版本>_amd64.deb
├── DEBIAN/
│   ├── control          Depends: dde-shell (>= 2.0) | dde-dock (>= 5.0)
│   ├── postinst         检测平台 → 只启用匹配的那一套
│   └── prerm            卸载时清理符号链接
├── usr/lib/x86_64-linux-gnu/dde-shell/
│   └── org.deepin.ds.dock.eye.so         ← V25 用（Qt6，原样保留）
├── usr/lib/dde-shell-eye-plugin/
│   ├── v20/libcartoon-eye.so             ← V20 用（Qt5，neutral 暂存位置）
│   └── restart-dock.sh                   ← 重启任务栏的公共脚本
├── usr/share/dde-shell/org.deepin.ds.dock.eye/   ← V25 的 QML + 数据
└── usr/share/dde-dock/cartoon-eye/               ← V20 的数据 + plugin.json
```

关键设计：

- **依赖用"或"关系**：`dde-shell (>= 2.0) | dde-dock (>= 5.0)`，两代 deepin 都能满足。
  （注意：或关系的每一项**不能**再加一层括号，否则 `dpkg-deb --build` 会报"缺少软件包名"。）
- **V20 的 `.so` 不直接装进 `/usr/lib/dde-dock/plugins/`**。原因有二：
  1. 该目录在 deepin 25 上**同样存在**，直接装进去会被 V25 的托盘加载器扫到，Qt5 插件在 Qt6 进程里必然加载失败并刷一堆错误日志；
  2. 由 `postinst` 建**符号链接**而不是装实体文件，这样增删链接不会破坏 dpkg 的文件清单，升级也无需特殊处理。
- **`postinst` 判定平台**的逻辑（踩过坑，见脚本内注释）：
  1. 有 `dde-shell` 命令 → V25；
  2. 否则存在 `libdde-shell.so.*` → V25（注意 V25 的 soname 是 `libdde-shell.so.1`，**不是** `.so.2`，别拿版本后缀猜）；
  3. 否则有 `dde-dock` 命令或 `/usr/lib/dde-dock/plugins/` 目录 → V20；
  4. 兜底读 `/etc/deepin-version`；
  5. 都不匹配 → `unknown`，什么都不动。
- `postinst`/`prerm` 刻意**不**用 `set -e`，任何一步失败都不让 dpkg 卡在 half-configured。

---

## 4. 构建

### 4.1 V25（dde-shell / Qt6）

在 deepin V25 或装有 Qt6 + dde-shell 开发头文件的机器上：

```bash
cd linux
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
# 产物：build/plugins/org.deepin.ds.dock.eye.so
```

### 4.2 V20（dde-dock / Qt5）

有两条路，产物都在 `v20/prebuilt/libcartoon-eye.so`。

**路线 A：在一台 deepin V20 机器上原生编译（推荐，最可靠）**

```bash
sudo apt install build-essential cmake qtbase5-dev dde-dock-dev
cd linux/v20
bash build-v20.sh            # 内部会定位 dde-dock 头文件并 cmake 构建
```

**路线 B：在任意机器上交叉编译（本仓库 `prebuilt/` 里的就是这么做出来的）**

```bash
cd linux/v20
bash setup-sysroot.sh        # 从 archive.debian.org 拉 Debian buster 的 Qt 5.11.3
                             #  + ICU63 + double-conversion，从 deepin 源拉 dde-dock-dev
bash cross-build.sh          # 用 sysroot 的 moc + g++ 产出 .so，并自带 ABI 自检
```

sysroot 默认落在 `$HOME/.cache/dde-shell-eye-plugin/v20-sysroot`，可用 `V20_SYSROOT=` 覆盖。

> **为什么必须用 sysroot 而不是本机 Qt**
> deepin 20.0~20.8 = Qt 5.11.3 + glibc 2.28。用高版本工具链编出来的 `.so` 会引用
> `GLIBC_2.38` 之类的符号版本，V20 加载时直接失败。
> 一个典型的坑：`std::fmod` 在本机 glibc 2.38 上会绑定成 `fmod@GLIBC_2.38`，
> 所以 `eyeartist.cpp` 里改用自写的 `modPos()` 替代（已确认产物**无 fmod 引用**）。

**产物 ABI 必须满足**（`cross-build.sh` 会自动校验）：

```
NEEDED: libQt5Core.so.5  libQt5Gui.so.5  libQt5Widgets.so.5  libQt5Network.so.5
        libstdc++.so.6   libm.so.6       libgcc_s.so.1       libc.so.6
最高 GLIBC 符号版本: GLIBC_2.14      （< 2.28 ✓）
最高 Qt  符号版本  : Qt_5.11         （= V20 基线 ✓）
```

### 4.3 打包

```bash
cd linux
bash packaging/make-dual-deb.sh              # 版本号自动读 v20/CMakeLists.txt
bash packaging/make-dual-deb.sh 1.9.8        # 或显式指定
# 产物：dist/dde-shell-eye-plugin_<版本>_amd64.deb
```

脚本会在打包前**硬性校验**，防止再出现"V20 那半是空的"：

- V20 的 `.so` 找不到 → 直接报错退出；
- V20 的 `.so` 里找不到 `com.deepin.dock.PluginsItemInterface` → 报错退出；
- V20 的 `.so` 链接了 `libdde-shell` → 报错退出（说明拿错成 V25 的产物了）。

---

## 5. 测试

两套回归测试，都不需要图形界面：

```bash
# 1) 部署脚本回归：伪造 V25 / V20 / 类 V23 / 非 DDE 等 7 个场景，跑 postinst + prerm
bash linux/packaging/test-deploy.sh
# 期望：结果：通过 12 项，失败 0 项

# 2) V20 插件加载冒烟测试：复刻 dde-dock 的两道加载关卡，离屏跑真 .so
V20_SYSROOT=$HOME/.cache/dde-shell-eye-plugin/v20-sysroot bash linux/v20/smoke-test.sh
# 期望：==== 全部通过
```

`smoke-test.sh` 复用 dde-dock 源码里的同一套逻辑：

- `QPluginLoader::metaData()` 的 `api` 是否在 `{1.1.1, 1.2, 1.2.1, DOCK_PLUGIN_API_VERSION}` 白名单内；
- `instance()` 是否成功、`qobject_cast<PluginsItemInterface*>` 是否非空（即 IID 匹配）；
- `itemWidget()` 的 `sizeHint()` 是否落在 `16~100` 的**正方形**（否则任务栏上会显示异常）。

---

## 6. 已知限制

- **未在真实 deepin V20 机器上做最终安装验证。** 本仓库的 V20 产物是用 **deepin 官方仓库
  的 `dde-dock-dev` 头文件**交叉编译的，加载路径、接口 ABI、api 白名单、`sizeHint`
  都在本机（deepin 25）以离屏方式验证通过；但"在 deepin 20 上 `dpkg -i` 后眼睛真的出现在
  任务栏上"这一步，需要在 V20 上实测一次。若在 V20 上跑 `bash v20/build-v20.sh` 原生编译，
  可完全排除交叉编译的残余风险。
- V20 的 dde-dock **不支持热加载插件**（`PluginLoader` 里没有 `QFileSystemWatcher`，
  也没有 reload 的 D-Bus 方法），所以 `postinst` 会尝试重启任务栏。重启脚本会先尝试
  直接 `kill`，等会话管理器自动拉起；如果不拉起，再用从旧进程 `/proc/PID/environ` 抓到的
  会话环境重新拉起。
- V20 的 `dde-dock` 没有 dde-shell 那种独立的弹窗机制，因此学习弹窗、右键菜单都是
  在 QWidget 里自己画的（`eyepopup.cpp` / `eyemenu.cpp`）。

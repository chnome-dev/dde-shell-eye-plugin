# 卡通眼珠 Windows 版 (CartoonEye)

deepin/UOS 任务栏卡通眼珠插件的 **Windows 11 移植版**：桌面悬浮卡通眼睛，跟随鼠标移动（GEyes 算法），左键弹出学习内容，右键切换样式。

> Linux 版（deepin 任务栏插件）在 `linux/` 目录，**同一个 deb 同时支持
> deepin V25 / V23（dde-shell）与 deepin V20 / V15（dde-dock）**。
> 从源码构建、打包、测试的完整说明见 [`linux/PACKAGING.md`](linux/PACKAGING.md)。

## ✨ 功能

- **20 套眼睛主题** + **宠物模式**（十二生肖·鱼·猫，emoji 动画）
- **鼠标跟随**：瞳孔椭圆约束 + 中心死区（参照 GNOME GEyes）
- **动画**：随机眨眼、空闲东张西望
- **学习内容**（单击眼睛弹出，加权随机、出现过的降频）：
  - 📜 唐诗三百首（含注释）、宋词三百首（含赏析）、诗经（含注释）
  - 🔤 雅思/托福/四六级/高考/2000词（含注释和例句）
  - 📖 维基百科精选词条（10030 条中文简介）
  - 🔊 Windows 系统语音朗读（SAPI）、📋 一键复制、🎲 换一个
- **显示位置**：四角快捷定位 / 自由拖动（记忆）
- **深浅色**跟随 Windows 主题

## 🚀 运行（绿色免安装）

1. 下载 Release 中的 `CartoonEye-win-x64.zip`（或自行编译）
2. 解压，双击 `CartoonEye.exe`
3. 右键眼睛 → 退出，可结束程序

> 学习数据在 `data/` 目录（10 个 JSON 文件），与 exe 同级，勿删除。

## 🔧 交叉编译（在 Linux 上编译 Windows 版）

依赖：
- MinGW-w64：`sudo apt install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64`
- Windows 版 Qt 6.8（mingw）：`pip install aqtinstall && aqt install-qt windows desktop 6.8.3 win64_mingw -m qtspeech qtmultimedia`
- host Qt6 工具（moc 等，用于 AUTOMOC）：`sudo apt install qt6-base-dev-tools`

```bash
cmake -B build-win -S . \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain-mingw.cmake \
  -DCMAKE_PREFIX_PATH=/path/to/qt/6.8.3/mingw_64 \
  -DQT_HOST_PATH=/usr \
  -DCMAKE_AUTOMOC_EXECUTABLE=/usr/lib/qt6/libexec/moc \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j$(nproc)
# 部署（手动复制 DLL + plugins + data 到 dist-win/，见部署说明）
```

## 📁 结构

```
├── CMakeLists.txt           # Qt6 Widgets 构建
├── toolchain-mingw.cmake    # MinGW 交叉编译工具链
├── main.cpp                 # 入口（加载 data/、显示主窗口）
├── eyewindow.{h,cpp}        # 眼睛悬浮窗（绘制/跟随/菜单/宠物）
├── learncore.{h,cpp}        # 学习数据核心（加载/加权随机/分类/TTS）
├── learnwindow.{h,cpp}      # 学习内容弹窗（显示/复制/朗读）
├── app.rc / eye.ico         # Windows 资源与图标
└── data/*.json              # 学习数据（10 个分类）
```

## 🐧 Linux 版（deepin 任务栏插件）

代码在 `linux/`，一个 deb 同时兼容两代 deepin：

| | deepin V25 / V23 | deepin V20 / V15 |
|---|---|---|
| 任务栏 | `dde-shell` 2.x（Qt6） | `dde-dock` 5.x（Qt5） |
| 实现 | QML + C++ applet | 纯 QWidget / QPainter |
| 插件路径 | `/usr/lib/x86_64-linux-gnu/dde-shell/` | `/usr/lib/dde-dock/plugins/` |

包内同时含两套插件，`postinst` 自动检测系统并**只启用匹配的那一套**
（V20 那套通过符号链接挂到 `/usr/lib/dde-dock/plugins/`，因此不会在 V25 上被误加载）。

```bash
# 构建 V25 插件
cd linux && cmake -B build && cmake --build build -j$(nproc)

# 构建 V20 插件（在 deepin V20 上原生编译；或交叉编译，见 PACKAGING.md）
cd linux/v20 && bash build-v20.sh

# 打成单 deb
cd linux && bash packaging/make-dual-deb.sh
```

> ⚠️ **1.9.6 及更早版本在 deepin V20 上无法安装** —— 那些包只包含 dde-shell 版实现，
> 且 `Depends` 写死了 `dde-shell / libdde-shell / libdde-shell-dock >= 2.0`。
> 根因分析与修复说明见 [`linux/PACKAGING.md`](linux/PACKAGING.md)。

## 📄 License

GPL-3.0-or-later

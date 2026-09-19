# 卡通眼珠 DDE Dock 插件 (dde-shell-eye-plugin)

在 deepin/UOS v25 任务栏（Dock）上显示一对卡通眼珠（当前版本 **v1.9.6**，另有 [Windows 11 版](https://github.com/chnome-dev/dde-shell-eye-plugin/tree/windows)：桌面悬浮卡通眼睛 CartoonEye），眼珠会跟随鼠标移动（算法参照 GNOME `gnome-applets` 的 GEyes），并支持单击弹出学习内容、右键切换样式。

## ✨ 功能特性

- **多主题眼睛**：20 套眼睛主题（蓝色卡通、双眼睛、绿眸/粉眸少女、浓睫大眼、恶魔红眼、恐怖之眼、南瓜怪眼、奇异双瞳、猫瞳、哆啦A梦、圆眼双瞳等）
- **动画效果**：平滑过渡跟随、随机眨眼（普通/快速/双连眨）、空闲时东张西望、瞳孔到边缘极限眯眼
- **宠物模式**：十二生肖 + 鱼 + 猫（带动画）
- **学习内容**（单击眼睛弹出，加权随机、出现过的降低频率）：
  - 📜 诗歌：唐诗三百首（含注释）、宋词三百首（含赏析·注释）、诗经（含注释）
  - 🔤 英语：雅思/托福/大学四级/六级/高考核心词汇/常用英语2000词（含注释和例句）
  - 📖 百科：维基百科精选词条（10030 条中文简介）
  - 支持网络 TTS 朗读（有道→百度回退）、一键复制、深浅色跟随系统
- **显示位置**：左 / 中 / 右（居中=启动器右侧）可调
- **系统集成**：在"插件区域"可勾选显隐

## 📁 项目结构

```
├── CMakeLists.txt          # 构建脚本
├── eyeapplet.h / .cpp      # 后端（DAppletDock、学习数据加载、TTS、状态持久化）
└── package/
    ├── main.qml            # 前端界面（眼睛渲染、菜单、学习弹窗）
    ├── metadata.json       # 插件元数据
    ├── icons/eye.svg       # 图标
    └── data/*.json         # 学习数据（10 个分类）
```

## 🔧 构建

依赖：Qt6 (Core Gui Network)、dde-shell 开发包、dde-shell-dock 开发包。

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/dde-dev
cmake --build build -j$(nproc)
```

## 📦 安装

```bash
sudo dpkg -i dde-shell-eye-plugin_1.9.6_amd64.deb
sudo apt -f install   # 如提示依赖缺失
```

安装后 postinst 脚本会自动重启任务栏（`systemctl --user restart dde-shell@DDE`）。

要求：**deepin/UOS v25**（dde-shell >= 2.0）、**x86_64** 架构。

## 🗃️ 学习数据来源

| 数据 | 来源 |
|---|---|
| 唐诗三百首 | gushicionline/tangPoems300 |
| 宋词三百首（赏析·注释） | gushiwen.cn 宋词精选（含译文/赏析） |
| 诗经（注释） | eanzhao/shijing（305 篇全注释） |
| 英语词汇 | kajweb/dict |
| 维基百科精选词条 | dumps.wikimedia.org 中文维基百科快照（2026-09） |

## 📄 License

GPL-3.0-or-later

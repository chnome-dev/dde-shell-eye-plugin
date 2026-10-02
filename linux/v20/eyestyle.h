// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 弹窗 / 菜单配色：对齐 V25 main.qml 的深色与浅色两套取值。
// 深色判定优先读 deepin 的外观配置，失败时退回 QPalette。

#pragma once

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
#include <QSettings>
#include <QString>

namespace EyeStyle {

// 是否深色外观
inline bool darkMode()
{
    // deepin 20 的 DDE 外观配置
    const QString path = QStringLiteral("/usr/share/dde-appearance/dde-appearance.conf");
    Q_UNUSED(path)
    // 1) dconf（DDE 使用 gsettings 后端）
    QSettings gs(QStringLiteral("/etc/dconf/db/dde-appearance.d/00-dde-appearance"),
                 QSettings::IniFormat);
    const QString theme = gs.value(QStringLiteral("theme/background")).toString();
    if (!theme.isEmpty())
        return theme.compare(QLatin1String("dark"), Qt::CaseInsensitive) == 0
               || theme.compare(QLatin1String("Dark"), Qt::CaseInsensitive) == 0;

    // 2) 兜底：用调色板亮度判断（dde-qt5integration 会按主题设置调色板）
    const QColor win = QApplication::palette().color(QPalette::Window);
    return win.lightness() < 128;
}

inline QColor popupBg()     { return darkMode() ? QColor("#232529") : QColor("#F7F9FC"); }
inline QColor popupBorder() { return darkMode() ? QColor("#3A3D42") : QColor(0, 0, 0, 31); }
inline QColor textMain()    { return darkMode() ? QColor("#E8EAED") : QColor("#1F2329"); }
inline QColor textBody()    { return darkMode() ? QColor("#C9CDD4") : QColor("#333333"); }
inline QColor textMuted()   { return darkMode() ? QColor("#8E939B") : QColor("#777777"); }
inline QColor textSub()     { return darkMode() ? QColor("#9AA0A8") : QColor("#8A919F"); }
inline QColor accent()      { return darkMode() ? QColor("#5B8FD9") : QColor("#3E6FB0"); }
inline QColor btnBg()       { return darkMode() ? QColor("#34373D") : QColor("#E8EDF3"); }
inline QColor btnHover()    { return darkMode() ? QColor("#41454D") : QColor("#DCE4EE"); }
inline QColor divider()     { return darkMode() ? QColor("#34373D") : QColor("#E4E7ED"); }
inline QColor rowHover()    { return darkMode() ? QColor("#3A3E45") : QColor("#E9EEF5"); }

} // namespace EyeStyle

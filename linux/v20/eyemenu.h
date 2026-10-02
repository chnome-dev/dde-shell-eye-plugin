// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// V20 版两级设置菜单（对应 V25 main.qml 的 eyeMenu / PanelMenu）。
// 根菜单固定 225px 高，二级菜单按内容自适应（上限 520，超出滚动）。

#pragma once

#include <QRect>
#include <QVector>
#include <QWidget>

class EyePlugin;

class EyeMenu : public QWidget
{
    Q_OBJECT
public:
    explicit EyeMenu(EyePlugin *plugin, QWidget *parent = nullptr);

    // anchor = 插件图标屏幕矩形；dockPosition = Dock::Position
    void popupAt(const QRect &anchor, int dockPosition);

signals:
    void aboutRequested();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    enum Action {
        ActNone = 0,
        ActLevel,
        ActEyeStyle,
        ActPetStyle,
        ActPosPref,
        ActAbout,
        ActBack,
        ActClose,
    };

    struct Row
    {
        QString label;
        bool header = false;
        bool checked = false;
        bool arrow = false;
        int action = ActNone;
        int arg = 0;
        QRect rect;
        bool hovered = false;
    };

    void setLevel(int lv);
    void rebuildRows();
    void relayout();
    void placeAt(const QRect &anchor, int dockPosition);
    void activate(const Row &row);

    EyePlugin *m_plugin = nullptr;
    QVector<Row> m_rows;
    QRect m_backRect;
    QRect m_closeRect;
    int m_level = 0;
    int m_scroll = 0;
    int m_contentH = 0;
    int m_pressedRow = -1;
    bool m_pressedHeader = false;
    int m_hoverHeader = 0;   // 1=back 2=close
};

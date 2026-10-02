// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// V20 版卡片弹窗：学习内容弹窗与关于弹窗。
// 对应 V25 main.qml 里的 PanelPopup（learnPopup / aboutPopup），
// 这里用无边框 Qt::Popup 窗口 + QTextBrowser 渲染富文本实现。

#pragma once

#include <QRect>
#include <QVector>
#include <QWidget>

class QTextBrowser;
class QTimer;
class EyePlugin;

class EyePopup : public QWidget
{
    Q_OBJECT
public:
    explicit EyePopup(EyePlugin *plugin, QWidget *parent = nullptr);

    // anchor = 插件图标在屏幕坐标系中的矩形；dockPosition = Dock::Position
    void showLearning(const QRect &anchor, int dockPosition);
    void showAbout(const QRect &anchor, int dockPosition);
    void showNextLearning();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    struct Btn
    {
        QString id;
        QString text;
        QRect rect;
        bool hovered = false;
    };

    void rebuild();
    void recreate();
    void applyContent();
    void layoutButtons();
    void placeAt(const QRect &anchor, int dockPosition);
    int indexAt(const QPoint &pos) const;

    EyePlugin *m_plugin = nullptr;
    QTextBrowser *m_view = nullptr;
    QTimer *m_copyTimer = nullptr;

    bool m_about = false;
    bool m_copyFlash = false;
    QString m_chip;
    QVector<Btn> m_btns;
    QVector<Btn> m_footerBtns;
    int m_pressed = -1;
    int m_pressedFooter = -1;
};

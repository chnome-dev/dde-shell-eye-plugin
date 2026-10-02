// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// V20 版任务栏眼睛控件：绘制眼睛/宠物、跟随时钟、眨眼、空闲游走、
// 左键学习弹窗、右键设置菜单。

#pragma once

#include <QElapsedTimer>
#include <QWidget>

class QTimer;
class EyePlugin;
class EyeMenu;
class EyePopup;

class EyeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EyeWidget(EyePlugin *plugin, QWidget *parent = nullptr);
    ~EyeWidget() override;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void tick();

private:
    void updateFollow();
    void updateBlink();
    qreal dockSize() const;
    QRect itemAnchorGlobal() const;

    EyePlugin *m_plugin = nullptr;
    QTimer *m_timer = nullptr;
    EyeMenu *m_menu = nullptr;
    EyePopup *m_popup = nullptr;

    QElapsedTimer m_clock;

    // 跟随
    qreal m_dx = 0;
    qreal m_dy = 0;
    qreal m_dxL = 0;
    qreal m_dyL = 0;
    qreal m_dxR = 0;
    qreal m_dyR = 0;
    qreal m_squint = 1.0;

    // 空闲游走
    qreal m_lastCursorX = -9999;
    qreal m_lastCursorY = -9999;
    qint64 m_lastMoveMs = 0;
    bool m_idleWander = false;
    qreal m_wanderX = 0;
    qreal m_wanderY = 0;
    qint64 m_nextWanderMs = 0;

    // 眨眼
    qreal m_blink = 1.0;
    int m_blinkKind = 0;      // 0 普通 1 快速 2 双连
    qint64 m_blinkStartMs = -100000;
    qint64 m_nextBlinkMs = 0;

    QPoint m_pressPos;
    bool m_pressValid = false;
};

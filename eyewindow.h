// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QPoint>
#include <QWidget>

class LearnCore;
class LearnWindow;
class QMenu;
class QTimer;

/** 眼睛主题参数 */
struct EyeTheme {
    QString name;
    QColor sclera;   // 眼白
    QColor iris;     // 虹膜
    QColor pupil;    // 瞳孔
    bool   doubleEye = false; // 👀 双眼
    bool   catPupil  = false; // 猫瞳（竖条）
    bool   demon     = false; // 红眼恶魔
    bool   twin      = false; // 双瞳仁
    bool   lash      = false; // 睫毛
    qreal  irisScale = 0.42;  // 虹膜相对眼眶比例
};

/** 主悬浮窗：卡通眼睛，跟随鼠标，左键学习、右键菜单 */
class EyeWindow : public QWidget
{
    Q_OBJECT
public:
    explicit EyeWindow(LearnCore *core);

    void showAtPreferred();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

private slots:
    void onTick();          // 鼠标轮询 + 眨眼/游走计时
    void onBlinkFrame();    // 眨眼动画帧
    void onWanderFrame();   // 空闲游走动画帧

private:
    void loadThemes();
    void buildMenu();
    void refreshWindow();   // 根据模式/主题刷新尺寸与样式
    QPointF pupilOffset(const QPointF &local, const QRectF &eyeRect, qreal irisR) const;
    void drawEye(QPainter &p, const QRectF &rect, const EyeTheme &t, qreal blink);
    void drawPet(QPainter &p);
    void updatePetAnim();

    LearnCore *m_core;
    LearnWindow *m_learn;

    QList<EyeTheme> m_themes;
    int m_eyeStyle = 0;
    int m_petStyle = -1;   // -1=眼睛, 0-13=宠物
    int m_posPref = 3;     // 0左上 1右上 2左下 3右下 4自由

    QTimer *m_tickTimer;
    QPoint m_lastCursor;
    qint64 m_lastMoveMs = 0;

    // 眨眼
    QTimer *m_blinkTimer;
    qreal m_blink = 0.0;
    bool m_blinkDir = false;
    // 空闲游走
    QTimer *m_wanderTimer;
    QPointF m_wanderTarget;
    QPointF m_wanderPos;
    bool m_wandering = false;
    qreal m_petPhase = 0.0;

    QPoint m_dragOffset;
    bool m_dragging = false;
    QPoint m_pressPos;
    bool m_pressed = false;
    QMenu *m_menu = nullptr;
};

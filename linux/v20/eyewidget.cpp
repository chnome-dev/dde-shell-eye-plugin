// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eyewidget.h"

#include "eyeartist.h"
#include "eyemenu.h"
#include "eyeplugin.h"
#include "eyepopup.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>

namespace {

// 眨眼分段：{时长ms, 目标纵比}
struct BlinkSeg
{
    int dur;
    qreal to;
};

const QVector<BlinkSeg> &blinkSegs(int kind)
{
    static const QVector<BlinkSeg> normal{{85, 0.06}, {120, 1.0}};
    static const QVector<BlinkSeg> fast{{45, 0.06}, {60, 1.0}};
    static const QVector<BlinkSeg> dbl{{70, 0.06}, {45, 1.0}, {40, 1.0}, {70, 0.06}, {120, 1.0}};
    switch (kind) {
    case 1: return fast;
    case 2: return dbl;
    default: return normal;
    }
}

qreal easeInOutQuad(qreal t)
{
    return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
}

qreal easeOutCubic(qreal t)
{
    const qreal u = 1.0 - t;
    return 1.0 - u * u * u;
}

} // namespace

EyeWidget::EyeWidget(EyePlugin *plugin, QWidget *parent)
    : QWidget(parent)
    , m_plugin(plugin)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setContentsMargins(0, 0, 0, 0);
    // 尺寸完全由 sizeHint() 决定（一个正方形），不要被布局拉伸。
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_clock.start();
    m_lastMoveMs = m_clock.elapsed();
    m_nextBlinkMs = m_clock.elapsed() + 3000;
    m_nextWanderMs = m_clock.elapsed() + 1800;

    m_timer = new QTimer(this);
    m_timer->setInterval(33);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &EyeWidget::tick);
    m_timer->start();

    connect(m_plugin, &EyePlugin::styleChanged, this, [this]() { update(); });
}

EyeWidget::~EyeWidget() = default;

QSize EyeWidget::sizeHint() const
{
    // dde-dock 通过 PluginsItem::sizeHint() -> 本函数的返回值来决定这个格子多宽多高，
    // 而且它会在控件「还没被布局」时（此时自身 height() 为 0）就来问，
    // 所以这里不能依赖自身尺寸，要取面板在「厚度」方向上的实际尺寸：
    //   Top/Bottom -> 面板高度；Left/Right -> 面板宽度。
    int s = 0;
    if (const QWidget *w = window()) {
        const int pos = int(m_plugin->dockPosition());
        s = (pos == int(Dock::Left) || pos == int(Dock::Right)) ? w->width() : w->height();
        s -= 4;  // 留一点余量，避免贴边或被裁掉
    }
    // dde-dock 面板厚度范围是 40~100（MAINWINDOW_MIN_SIZE/MAX_SIZE），
    // 取不到窗口或数值异常时回退到一个安全的方形。
    if (s < 16 || s > 100)
        s = 36;
    return QSize(s, s);
}

qreal EyeWidget::dockSize() const
{
    return qMax(1, qMin(width(), height()));
}

QRect EyeWidget::itemAnchorGlobal() const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size());
}

// ---------------------------------------------------------------------------
// 每帧：跟随 + 眨眼 + 重绘
// ---------------------------------------------------------------------------
void EyeWidget::tick()
{
    updateFollow();
    updateBlink();
    update();
}

void EyeWidget::updateFollow()
{
    const qreal dpr = devicePixelRatioF() > 0 ? devicePixelRatioF() : 1.0;
    const QPoint raw = m_plugin->cursorPos();
    const qreal gx = raw.x() / dpr;
    const qreal gy = raw.y() / dpr;

    const qint64 now = m_clock.elapsed();

    if (qAbs(gx - m_lastCursorX) > 0.5 || qAbs(gy - m_lastCursorY) > 0.5) {
        m_lastCursorX = gx;
        m_lastCursorY = gy;
        m_lastMoveMs = now;
        if (m_idleWander)
            m_idleWander = false;
    } else if (!m_idleWander && now - m_lastMoveMs > 4000) {
        m_idleWander = true;
        m_nextWanderMs = now;
    }

    if (m_idleWander && now >= m_nextWanderMs) {
        const qreal w = width();
        const qreal h = height();
        m_wanderX = w / 2 + (QRandomGenerator::global()->generateDouble() - 0.5) * w * 1.8;
        m_wanderY = h / 2 + (QRandomGenerator::global()->generateDouble() - 0.5) * h * 1.5;
        m_nextWanderMs = now + 1800;
    }

    // 把全局光标换算到「以控件为中心、边长 dockSize 的方形」内的局部坐标
    const qreal ds = dockSize();
    const QPointF localPt = QPointF(gx, gy) - QPointF(mapToGlobal(QPoint(0, 0)));
    const qreal ox = (width() - ds) / 2.0;
    const qreal oy = (height() - ds) / 2.0;
    qreal lx = localPt.x() - ox;
    qreal ly = localPt.y() - oy;
    if (m_idleWander) {
        lx = m_wanderX - ox;
        ly = m_wanderY - oy;
    }

    const int themeIdx = qBound(0, m_plugin->eyeStyle(), eyeThemeCount() - 1);
    const EyeTheme &theme = eyeThemes().at(themeIdx);

    const qreal wall = qMax(2.0, ds * 0.07);

    // 单眼
    const QPointF off0 = computePupilOffset(lx, ly, ds / 2, ds / 2, ds / 2, ds / 2,
                                            ds * 0.29, wall);
    m_dx = off0.x();
    m_dy = off0.y();

    // 双眼
    const qreal eyeW = ds * (theme.round ? 0.47 : 0.46);
    const qreal eyeH = theme.round ? ds * 0.95 : ds;
    const qreal cy = ds / 2;
    const qreal lcx = ds * 0.25;
    const qreal rcx = ds * 0.75;
    const qreal irisR = eyeW * theme.irisScale / 2.0 * 1.15;
    const QPointF offL = computePupilOffset(lx, ly, lcx, cy, eyeW / 2, eyeH / 2, irisR, wall);
    const QPointF offR = computePupilOffset(lx, ly, rcx, cy, eyeW / 2, eyeH / 2, irisR, wall);
    m_dxL = offL.x();
    m_dyL = offL.y();
    m_dxR = offR.x();
    m_dyR = offR.y();

    const qreal ratio0 = qSqrt(off0.x() * off0.x() + off0.y() * off0.y())
                         / qMax(1.0, ds / 2 - ds * 0.29 - wall);
    const qreal ratio1 = qMax(qSqrt(offL.x() * offL.x() + offL.y() * offL.y()),
                              qSqrt(offR.x() * offR.x() + offR.y() * offR.y()))
                         / qMax(1.0, eyeW / 2 - irisR - wall);
    const qreal ratio = theme.dbl ? ratio1 : ratio0;
    m_squint = ratio > 0.8 ? 0.88 : 1.0;
}

void EyeWidget::updateBlink()
{
    const qint64 now = m_clock.elapsed();

    if (now >= m_nextBlinkMs) {
        const double r = QRandomGenerator::global()->generateDouble();
        if (r < 0.18)
            m_blinkKind = 1;
        else if (r < 0.38)
            m_blinkKind = 2;
        else
            m_blinkKind = 0;
        m_blinkStartMs = now;
        m_nextBlinkMs = now + 2200 + qint64(QRandomGenerator::global()->generateDouble() * 4800);
    }

    const QVector<BlinkSeg> &segs = blinkSegs(m_blinkKind);
    int total = 0;
    for (const BlinkSeg &s : segs)
        total += s.dur;

    qint64 e = now - m_blinkStartMs;
    if (e < 0 || e >= total) {
        m_blink = 1.0;
        return;
    }

    qreal from = 1.0;
    for (const BlinkSeg &s : segs) {
        if (e < s.dur) {
            const qreal t = s.dur > 0 ? qreal(e) / qreal(s.dur) : 1.0;
            const bool closing = (s.to < from);
            const qreal k = closing ? easeInOutQuad(t) : easeOutCubic(t);
            m_blink = from + (s.to - from) * k;
            return;
        }
        e -= s.dur;
        from = s.to;
    }
    m_blink = 1.0;
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------
void EyeWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal ds = dockSize();
    const int pet = m_plugin->petStyle();

    if (pet >= 0) {
        drawPet(&p, QRectF(rect()), ds, pet, m_clock.elapsed());
        return;
    }

    const int themeIdx = qBound(0, m_plugin->eyeStyle(), eyeThemeCount() - 1);
    const EyeTheme &theme = eyeThemes().at(themeIdx);

    EyeFrame f;
    f.dx = m_dx;
    f.dy = m_dy;
    f.dxL = m_dxL;
    f.dyL = m_dyL;
    f.dxR = m_dxR;
    f.dyR = m_dyR;
    f.blink = m_blink;
    f.squint = m_squint;

    drawEye(&p, QRectF(rect()), ds, theme, f);
}

// ---------------------------------------------------------------------------
// 交互：左键学习弹窗 / 右键设置菜单
// ---------------------------------------------------------------------------
void EyeWidget::mousePressEvent(QMouseEvent *e)
{
    m_pressPos = e->pos();
    m_pressValid = true;
    // 自行处理，避免 dde-dock 弹出它自己的原生菜单
    e->accept();
}

void EyeWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_pressValid || (e->pos() - m_pressPos).manhattanLength() > 8) {
        m_pressValid = false;
        e->accept();
        return;
    }
    m_pressValid = false;

    if (!m_popup)
        m_popup = new EyePopup(m_plugin, this);
    if (!m_menu) {
        m_menu = new EyeMenu(m_plugin, this);
        connect(m_menu, &EyeMenu::aboutRequested, this, [this]() {
            m_popup->showAbout(itemAnchorGlobal(), int(m_plugin->dockPosition()));
        });
    }

    const QRect anchor = itemAnchorGlobal();
    const int dockPos = int(m_plugin->dockPosition());

    if (e->button() == Qt::RightButton) {
        m_popup->hide();
        m_menu->popupAt(anchor, dockPos);
    } else if (e->button() == Qt::LeftButton) {
        m_menu->hide();
        m_popup->showLearning(anchor, dockPos);
    }
    e->accept();
}

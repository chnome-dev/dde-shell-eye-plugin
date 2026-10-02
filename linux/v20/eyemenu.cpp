// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eyemenu.h"

#include "eyeartist.h"
#include "eyeplugin.h"
#include "eyestyle.h"

#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QWheelEvent>

namespace {
constexpr int kWidth = 250;
constexpr int kRadius = 12;
constexpr int kHeaderMargin = 10;
constexpr int kHeaderH = 30;
constexpr int kFlickMargin = 8;
constexpr int kRowH = 30;
constexpr int kHeaderRowH = 24;
constexpr int kRootHeight = 225;
constexpr int kMaxHeight = 520;

// Dock::Position 的真实取值（见 dde-dock interfaces/constants.h）：
//   Top = 0, Right = 1, Bottom = 2, Left = 3
// 注意顺序不是「上/下/左/右」，写成常量以免再搞错。
constexpr int kDockTop    = int(Dock::Top);
constexpr int kDockRight  = int(Dock::Right);
constexpr int kDockBottom = int(Dock::Bottom);
constexpr int kDockLeft   = int(Dock::Left);

int contentTop() { return kHeaderMargin + kHeaderH + kFlickMargin; }
} // namespace

EyeMenu::EyeMenu(EyePlugin *plugin, QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_plugin(plugin)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    connect(m_plugin, &EyePlugin::styleChanged, this, [this]() {
        if (!isVisible())
            return;
        rebuildRows();
        // 二级菜单的选中态/行数可能变化，重新按内容定高
        relayout();
    });
}

// ---------------------------------------------------------------------------
// 行定义
// ---------------------------------------------------------------------------
void EyeMenu::rebuildRows()
{
    m_rows.clear();

    auto add = [this](const QString &label, int action, int arg, bool checked,
                      bool header, bool arrow) {
        Row r;
        r.label = label;
        r.action = action;
        r.arg = arg;
        r.checked = checked;
        r.header = header;
        r.arrow = arrow;
        m_rows.append(r);
    };

    switch (m_level) {
    case 0:
        add(tr("眼睛样式"), ActLevel, 1, false, false, true);
        add(tr("宠物样式"), ActLevel, 3, false, false, true);
        add(tr("学习内容"), ActLevel, 2, false, false, true);
        add(tr("显示位置"), ActLevel, 4, false, false, true);
        add(tr("关于"), ActAbout, 0, false, false, false);
        break;

    case 1: {
        const QVector<EyeTheme> &themes = eyeThemes();
        for (int i = 0; i < themes.size(); ++i) {
            add(themes.at(i).name, ActEyeStyle, i,
                m_plugin->eyeStyle() == i && m_plugin->petStyle() < 0, false, false);
        }
        break;
    }

    case 2:
        add(tr("英文学习"), ActNone, 0, false, true, false);
        add(tr("雅思词汇（含注释和例句）"), ActNone, 3, m_plugin->categoryEnabled(3), false, false);
        add(tr("托福词汇（含注释和例句）"), ActNone, 4, m_plugin->categoryEnabled(4), false, false);
        add(tr("大学四级词汇（含注释和例句）"), ActNone, 5, m_plugin->categoryEnabled(5), false, false);
        add(tr("大学六级词汇（含注释和例句）"), ActNone, 6, m_plugin->categoryEnabled(6), false, false);
        add(tr("高考核心词汇（含注释和例句）"), ActNone, 7, m_plugin->categoryEnabled(7), false, false);
        add(tr("常用英语2000词（含注释和例句）"), ActNone, 8, m_plugin->categoryEnabled(8), false, false);
        add(tr("诗歌学习"), ActNone, 0, false, true, false);
        add(tr("唐诗三百首（含注释）"), ActNone, 0, m_plugin->categoryEnabled(0), false, false);
        add(tr("宋词三百首（含注释）"), ActNone, 1, m_plugin->categoryEnabled(1), false, false);
        add(tr("诗经（含注释）"), ActNone, 2, m_plugin->categoryEnabled(2), false, false);
        add(tr("百科学习"), ActNone, 0, false, true, false);
        add(tr("维基百科精选词条（含简介）"), ActNone, 9, m_plugin->categoryEnabled(9), false, false);
        break;

    case 3:
        add(tr("眼睛模式（关闭宠物）"), ActPetStyle, -1, m_plugin->petStyle() < 0, false, false);
        for (int i = 0; i < petCount(); ++i)
            add(petDefs().at(i).name, ActPetStyle, i, m_plugin->petStyle() == i, false, false);
        break;

    case 4:
        add(tr("左侧"), ActPosPref, 0, m_plugin->posPref() == 0, false, false);
        add(tr("居中（启动器右侧）"), ActPosPref, 1, m_plugin->posPref() == 1, false, false);
        add(tr("右侧"), ActPosPref, 2, m_plugin->posPref() == 2, false, false);
        break;

    default:
        break;
    }
}

void EyeMenu::setLevel(int lv)
{
    m_level = lv;
    m_scroll = 0;
    rebuildRows();
    relayout();
}

// ---------------------------------------------------------------------------
// 排布
// ---------------------------------------------------------------------------
void EyeMenu::relayout()
{
    // 计算内容高度
    m_contentH = 0;
    for (const Row &r : m_rows)
        m_contentH += (r.header ? kHeaderRowH : kRowH) + 2;

    int h;
    if (m_level == 0) {
        h = kRootHeight;
    } else {
        h = qBound(100, m_contentH + contentTop() + kFlickMargin, kMaxHeight);
    }
    resize(kWidth, h);

    int y = contentTop() - m_scroll;
    for (Row &r : m_rows) {
        const int rh = r.header ? kHeaderRowH : kRowH;
        r.rect = QRect(kFlickMargin, y, kWidth - kFlickMargin * 2, rh);
        y += rh + 2;
    }

    m_backRect = QRect(kHeaderMargin, kHeaderMargin - 3, 26, 24);
    m_closeRect = QRect(kWidth - kHeaderMargin - 26, kHeaderMargin - 3, 26, 24);

    const int maxScroll = qMax(0, m_contentH + contentTop() + kFlickMargin - height());
    m_scroll = qBound(0, m_scroll, maxScroll);
}

void EyeMenu::placeAt(const QRect &anchor, int dockPosition)
{
    int x = anchor.left();
    int y = anchor.top() - height() - 10;
    switch (dockPosition) {
    case kDockTop:
        x = anchor.left();
        y = anchor.bottom() + 10;
        break;
    case kDockLeft:
        x = anchor.right() + 10;
        y = anchor.top();
        break;
    case kDockRight:
        x = anchor.left() - width() - 10;
        y = anchor.top();
        break;
    default:
        x = anchor.left();
        y = anchor.top() - height() - 10;
        break;
    }

    QScreen *scr = QGuiApplication::screenAt(QCursor::pos());
    if (!scr)
        scr = QGuiApplication::primaryScreen();
    const QRect avail = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);
    x = qBound(avail.left() + 4, x, avail.right() - width() - 4);
    y = qBound(avail.top() + 4, y, avail.bottom() - height() - 4);
    move(x, y);
}

void EyeMenu::popupAt(const QRect &anchor, int dockPosition)
{
    m_level = 0;
    m_scroll = 0;
    rebuildRows();
    relayout();
    placeAt(anchor, dockPosition);
    show();
    raise();
    activateWindow();
    update();
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------
void EyeMenu::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(EyeStyle::popupBorder(), 1));
    p.setBrush(EyeStyle::popupBg());
    p.drawRoundedRect(card, kRadius, kRadius);

    // 顶栏
    if (m_level > 0) {
        p.setPen(QPen(EyeStyle::popupBorder(), 1));
        p.setBrush(m_hoverHeader == 1 ? EyeStyle::btnHover() : EyeStyle::btnBg());
        p.drawRoundedRect(m_backRect, 12, 12);
        QFont bf = font();
        bf.setPixelSize(16);
        p.setFont(bf);
        p.setPen(EyeStyle::textMain());
        p.drawText(m_backRect, Qt::AlignCenter, QStringLiteral("‹"));
    }

    QFont tf = font();
    tf.setPixelSize(13);
    tf.setBold(true);
    p.setFont(tf);
    p.setPen(EyeStyle::textMain());
    QString title;
    switch (m_level) {
    case 0: title = tr("设置"); break;
    case 1: title = tr("眼睛样式"); break;
    case 2: title = tr("学习内容"); break;
    case 3: title = tr("宠物样式"); break;
    case 4: title = tr("显示位置"); break;
    default: break;
    }
    p.drawText(QRect(m_backRect.right() + 6, kHeaderMargin - 3,
                     m_closeRect.left() - m_backRect.right() - 12, 24),
               Qt::AlignCenter, title);

    p.setPen(QPen(EyeStyle::popupBorder(), 1));
    p.setBrush(m_hoverHeader == 2 ? EyeStyle::btnHover() : EyeStyle::btnBg());
    p.drawRoundedRect(m_closeRect, 12, 12);
    QFont cf = font();
    cf.setPixelSize(12);
    p.setFont(cf);
    p.setPen(EyeStyle::textSub());
    p.drawText(m_closeRect, Qt::AlignCenter, QStringLiteral("✕"));

    // 内容区
    p.save();
    p.setClipRect(QRect(0, contentTop() - 4, width(), height() - contentTop() - kFlickMargin + 8));
    QFont rf = font();
    for (const Row &r : m_rows) {
        if (!r.rect.intersects(QRect(0, contentTop() - 8, width(), height() - contentTop() + 8)))
            continue;

        if (!r.header && r.hovered) {
            p.setPen(Qt::NoPen);
            p.setBrush(EyeStyle::rowHover());
            p.drawRoundedRect(r.rect, 6, 6);
        }

        rf.setPixelSize(r.header ? 11 : 13);
        rf.setBold(r.header);
        p.setFont(rf);
        p.setPen(r.header ? EyeStyle::accent() : EyeStyle::textMain());
        const QRect textRect = r.rect.adjusted(r.header ? 10 : 14, 0, -40, 0);
        const QString elided = QFontMetrics(rf).elidedText(r.label, Qt::ElideRight, textRect.width());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

        if (!r.header) {
            rf.setPixelSize(13);
            p.setFont(rf);
            p.setPen(EyeStyle::accent());
            const QString mark = r.arrow ? QStringLiteral("▸")
                                         : (r.checked ? QStringLiteral("✓") : QString());
            if (!mark.isEmpty()) {
                p.drawText(QRect(r.rect.right() - 26, r.rect.top(), 14, r.rect.height()),
                           Qt::AlignCenter, mark);
            }
        }
    }
    p.restore();
}

// ---------------------------------------------------------------------------
// 交互
// ---------------------------------------------------------------------------
void EyeMenu::activate(const Row &row)
{
    switch (row.action) {
    case ActLevel:
        setLevel(row.arg);
        update();
        return;
    case ActBack:
        setLevel(0);
        update();
        return;
    case ActClose:
        hide();
        return;
    case ActAbout:
        hide();
        Q_EMIT aboutRequested();
        return;
    case ActEyeStyle:
        m_plugin->setPetStyle(-1);
        m_plugin->setEyeStyle(row.arg);
        hide();
        return;
    case ActPetStyle:
        m_plugin->setPetStyle(row.arg);
        hide();
        return;
    case ActPosPref:
        m_plugin->setPosPref(row.arg);
        hide();
        return;
    case ActNone:
    default:
        // 学习分类：切换勾选
        if (!row.header) {
            m_plugin->setCategoryEnabled(row.arg, !m_plugin->categoryEnabled(row.arg));
            rebuildRows();
            update();
        }
        return;
    }
}

void EyeMenu::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    m_pressedRow = -1;
    m_pressedHeader = false;

    if (m_level > 0 && m_backRect.contains(e->pos())) {
        m_pressedHeader = true;
    } else if (m_closeRect.contains(e->pos())) {
        m_pressedHeader = true;
    } else {
        for (int i = 0; i < m_rows.size(); ++i) {
            if (m_rows.at(i).rect.contains(e->pos())) {
                m_pressedRow = i;
                break;
            }
        }
    }
    e->accept();
}

void EyeMenu::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(e);
        return;
    }

    const int pressedRow = m_pressedRow;
    const bool pressedHeader = m_pressedHeader;
    m_pressedRow = -1;
    m_pressedHeader = false;

    if (m_level > 0 && m_backRect.contains(e->pos())) {
        if (pressedHeader)
            setLevel(0);
        update();
        e->accept();
        return;
    }
    if (m_closeRect.contains(e->pos())) {
        if (pressedHeader)
            hide();
        e->accept();
        return;
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).rect.contains(e->pos())) {
            if (i == pressedRow)
                activate(m_rows.at(i));
            e->accept();
            return;
        }
    }
    e->accept();
}

void EyeMenu::mouseMoveEvent(QMouseEvent *e)
{
    int header = 0;
    if (m_level > 0 && m_backRect.contains(e->pos()))
        header = 1;
    else if (m_closeRect.contains(e->pos()))
        header = 2;

    bool need = (header != m_hoverHeader);
    m_hoverHeader = header;

    for (Row &r : m_rows) {
        const bool h = !r.header && r.rect.contains(e->pos());
        if (h != r.hovered) {
            r.hovered = h;
            need = true;
        }
    }
    if (need)
        update();
    QWidget::mouseMoveEvent(e);
}

void EyeMenu::leaveEvent(QEvent *e)
{
    m_hoverHeader = 0;
    for (Row &r : m_rows)
        r.hovered = false;
    update();
    QWidget::leaveEvent(e);
}

void EyeMenu::wheelEvent(QWheelEvent *e)
{
    const int maxScroll = qMax(0, m_contentH + contentTop() + kFlickMargin - height());
    if (maxScroll > 0) {
        const int delta = e->angleDelta().y() > 0 ? -40 : 40;
        m_scroll = qBound(0, m_scroll + delta, maxScroll);
        relayout();
        update();
    }
    e->accept();
}

void EyeMenu::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        if (m_level > 0) {
            setLevel(0);
            update();
        } else {
            hide();
        }
        e->accept();
        return;
    }
    QWidget::keyPressEvent(e);
}

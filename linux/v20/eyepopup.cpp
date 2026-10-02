// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eyepopup.h"

#include "eyeplugin.h"
#include "eyestyle.h"

#include <QApplication>
#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

namespace {
constexpr int kRadius = 14;
constexpr int kMargin = 14;
constexpr int kHeaderH = 28;
constexpr int kLearnWidth = 460;
constexpr int kAboutWidth = 340;
} // namespace

EyePopup::EyePopup(EyePlugin *plugin, QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_plugin(plugin)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    m_view = new QTextBrowser(this);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setStyleSheet(QStringLiteral("QTextBrowser{background:transparent;border:none;}"));
    m_view->setOpenLinks(false);
    m_view->setOpenExternalLinks(false);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->viewport()->setAutoFillBackground(false);
    connect(m_view, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        const QString s = url.toString();
        const QString scheme = QStringLiteral("eye-speak:");
        if (!s.startsWith(scheme))
            return;
        const QString payload = s.mid(scheme.size());
        const int bar = payload.indexOf(QLatin1Char('|'));
        if (bar <= 0)
            return;
        const QString lang = payload.left(bar);
        const QString text = QUrl::fromPercentEncoding(payload.mid(bar + 1).toUtf8());
        m_plugin->speak(text, lang);
    });

    m_copyTimer = new QTimer(this);
    m_copyTimer->setSingleShot(true);
    m_copyTimer->setInterval(1200);
    connect(m_copyTimer, &QTimer::timeout, this, [this]() {
        m_copyFlash = false;
        recreate();
        update();
    });

    connect(m_plugin, &EyePlugin::learningChanged, this, [this]() {
        if (isVisible() && !m_about) {
            applyContent();
            recreate();
        }
    });
}

// ---------------------------------------------------------------------------
// 按钮定义（对齐 V25：复制 / 换一个 / ✕；关于弹窗为单个「知道了」）
// ---------------------------------------------------------------------------
void EyePopup::rebuild()
{
    m_btns.clear();
    m_footerBtns.clear();
    m_pressed = -1;
    m_pressedFooter = -1;

    if (m_about)
        return;

    m_btns << Btn{QStringLiteral("copy"),
                  m_copyFlash ? tr("✓ 已复制") : tr("📋 复制"), QRect(), false};
    m_btns << Btn{QStringLiteral("next"), tr("🎲 换一个"), QRect(), false};
    m_btns << Btn{QStringLiteral("close"), QStringLiteral("✕"), QRect(), false};
}

void EyePopup::layoutButtons()
{
    QFontMetrics fm(font());
    int right = width() - kMargin;

    // 从右往左排：✕ / 换一个 / 复制
    for (int i = m_btns.size() - 1; i >= 0; --i) {
        Btn &b = m_btns[i];
        int w, h;
        if (b.id == QLatin1String("close")) {
            w = 26;
            h = 26;
        } else {
            w = fm.horizontalAdvance(b.text) + 20;
            h = 26;
        }
        b.rect = QRect(right - w, kMargin + (kHeaderH - h) / 2, w, h);
        right -= w + 8;
    }

    if (m_about) {
        m_footerBtns.clear();
        const int w = 90;
        const int h = 30;
        m_footerBtns << Btn{QStringLiteral("ok"), tr("知道了"),
                            QRect((width() - w) / 2, height() - kMargin - h, w, h), false};
    }
}

// ---------------------------------------------------------------------------
// 内容排布
// ---------------------------------------------------------------------------
void EyePopup::applyContent()
{
    const int contentW = (m_about ? kAboutWidth : kLearnWidth) - kMargin * 2;
    m_view->document()->setTextWidth(contentW);
    m_view->setHtml(m_about ? m_plugin->aboutHtml() : m_plugin->currentLearnHtml());
    m_view->document()->setTextWidth(contentW);
}

void EyePopup::recreate()
{
    rebuild();

    const int w = m_about ? kAboutWidth : kLearnWidth;
    resize(w, height());  // 先定宽，便于文档按宽度排版

    applyContent();

    int h;
    if (m_about) {
        h = 350;
    } else {
        const qreal docH = m_view->document()->size().height();
        h = int(qBound(260.0, 96.0 + docH, 470.0));
    }
    resize(w, h);

    layoutButtons();

    const int viewTop = kMargin + kHeaderH;
    const int viewBottom = m_about ? (height() - kMargin - 40) : (height() - kMargin);
    m_view->setGeometry(kMargin, viewTop, width() - kMargin * 2,
                        qMax(20, viewBottom - viewTop));

    // 正文配色跟随深浅色
    m_view->setStyleSheet(QStringLiteral("QTextBrowser{background:transparent;border:none;color:%1;}")
                              .arg(EyeStyle::textBody().name()));
    // 只读不可编辑但可选中
    m_view->setReadOnly(true);
    m_view->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
}

// ---------------------------------------------------------------------------
// 定位（对齐 V25 positionPopup）
// ---------------------------------------------------------------------------
void EyePopup::placeAt(const QRect &anchor, int dockPosition)
{
    // Dock::Position 的真实取值（dde-dock interfaces/constants.h）：
    //   Top = 0, Right = 1, Bottom = 2, Left = 3
    // 默认（Bottom）：弹窗浮在图标上方；Top 则在其下方；Left/Right 分别贴右/左侧。
    int x = anchor.left();
    int y = anchor.top() - height() - 10;
    switch (dockPosition) {
    case int(Dock::Top):  // 0
        x = anchor.left();
        y = anchor.bottom() + 10;
        break;
    case int(Dock::Right):  // 1
        x = anchor.left() - width() - 10;
        y = anchor.top();
        break;
    case int(Dock::Left):  // 3
        x = anchor.right() + 10;
        y = anchor.top();
        break;
    default:  // Bottom = 2
        x = anchor.left();
        y = anchor.top() - height() - 10;
        break;
    }

    // 限制在可用屏幕范围内
    QScreen *scr = QGuiApplication::screenAt(QCursor::pos());
    if (!scr)
        scr = QGuiApplication::primaryScreen();
    const QRect avail = scr ? scr->availableGeometry() : QRect(0, 0, 1920, 1080);
    x = qBound(avail.left() + 4, x, avail.right() - width() - 4);
    y = qBound(avail.top() + 4, y, avail.bottom() - height() - 4);
    move(x, y);
}

void EyePopup::showLearning(const QRect &anchor, int dockPosition)
{
    m_about = false;
    m_copyFlash = false;
    if (m_plugin->currentEmpty())
        m_plugin->pickNextLearningItem();
    recreate();
    placeAt(anchor, dockPosition);
    show();
    raise();
    activateWindow();
}

void EyePopup::showNextLearning()
{
    if (!isVisible() || m_about)
        return;
    m_plugin->pickNextLearningItem();
}

void EyePopup::showAbout(const QRect &anchor, int dockPosition)
{
    m_about = true;
    recreate();
    placeAt(anchor, dockPosition);
    show();
    raise();
    activateWindow();
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------
void EyePopup::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    p.setPen(QPen(EyeStyle::popupBorder(), 1));
    p.setBrush(EyeStyle::popupBg());
    p.drawRoundedRect(card, kRadius, kRadius);

    if (m_about)
        return;

    // 分类胶囊
    QFont f = font();
    f.setPixelSize(12);
    p.setFont(f);
    const QString chip = m_plugin->chipText();
    const int chipW = qMax(84, QFontMetrics(f).horizontalAdvance(chip) + 18);
    const QRectF chipRect(kMargin, kMargin + (kHeaderH - 24) / 2, chipW, 24);
    p.setPen(Qt::NoPen);
    p.setBrush(EyeStyle::accent());
    p.drawRoundedRect(chipRect, 12, 12);
    p.setPen(QColor("#FFFFFF"));
    p.drawText(chipRect, Qt::AlignCenter, chip);

    // 按钮
    for (const Btn &b : m_btns) {
        const bool isClose = (b.id == QLatin1String("close"));
        p.setPen(QPen(EyeStyle::popupBorder(), 1));
        p.setBrush(b.hovered ? EyeStyle::btnHover() : EyeStyle::btnBg());
        p.drawRoundedRect(b.rect, isClose ? 13 : 13, 13);

        QFont bf = font();
        bf.setPixelSize(12);
        p.setFont(bf);
        if (isClose)
            p.setPen(EyeStyle::textSub());
        else if (b.id == QLatin1String("copy") && m_copyFlash)
            p.setPen(QColor("#2EA043"));
        else
            p.setPen(EyeStyle::accent());
        p.drawText(b.rect, Qt::AlignCenter, b.text);
    }
}

// ---------------------------------------------------------------------------
// 交互
// ---------------------------------------------------------------------------
int EyePopup::indexAt(const QPoint &pos) const
{
    for (int i = 0; i < m_btns.size(); ++i) {
        if (m_btns.at(i).rect.contains(pos))
            return i;
    }
    return -1;
}

void EyePopup::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    m_pressed = indexAt(e->pos());
    m_pressedFooter = -1;
    for (int i = 0; i < m_footerBtns.size(); ++i) {
        if (m_footerBtns.at(i).rect.contains(e->pos()))
            m_pressedFooter = i;
    }
    e->accept();
}

void EyePopup::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(e);
        return;
    }

    int footer = -1;
    for (int i = 0; i < m_footerBtns.size(); ++i) {
        if (m_footerBtns.at(i).rect.contains(e->pos()))
            footer = i;
    }

    const int pressed = m_pressed;
    const int pressedFooter = m_pressedFooter;
    m_pressed = -1;
    m_pressedFooter = -1;

    if (footer >= 0 && footer == pressedFooter) {
        hide();
        e->accept();
        return;
    }

    const int idx = indexAt(e->pos());
    if (idx < 0 || idx != pressed) {
        e->accept();
        return;
    }

    const QString id = m_btns.at(idx).id;
    if (id == QLatin1String("close")) {
        hide();
    } else if (id == QLatin1String("next")) {
        m_plugin->pickNextLearningItem();
    } else if (id == QLatin1String("copy")) {
        m_plugin->copyText(m_plugin->currentPlainText());
        m_copyFlash = true;
        recreate();
        update();
        m_copyTimer->start();
    }
    e->accept();
}

void EyePopup::mouseMoveEvent(QMouseEvent *e)
{
    bool need = false;
    for (Btn &b : m_btns) {
        const bool h = b.rect.contains(e->pos());
        if (h != b.hovered) {
            b.hovered = h;
            need = true;
        }
    }
    if (need)
        update();
    QWidget::mouseMoveEvent(e);
}

void EyePopup::leaveEvent(QEvent *e)
{
    for (Btn &b : m_btns)
        b.hovered = false;
    update();
    QWidget::leaveEvent(e);
}

void EyePopup::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        hide();
        e->accept();
        return;
    }
    QWidget::keyPressEvent(e);
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "learnwindow.h"
#include "learncore.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollBar>
#include <QStyleHints>
#include <QTextBrowser>
#include <QVBoxLayout>

static bool systemDark()
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

LearnWindow::LearnWindow(LearnCore *core, QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_core(core)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(460);
    setMinimumHeight(220);
    setMaximumHeight(560);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 14, 14, 14);

    // 顶栏
    auto *header = new QHBoxLayout;
    m_catLabel = new QLabel(QStringLiteral("学习内容"), this);
    m_catLabel->setStyleSheet(QStringLiteral(
        "background:#3E6FB0;color:#fff;border-radius:11px;padding:3px 12px;font-size:12px;"));
    header->addWidget(m_catLabel);
    header->addStretch();
    m_copyBtn = new QPushButton(QStringLiteral("📋 复制"), this);
    m_nextBtn = new QPushButton(QStringLiteral("🎲 换一个"), this);
    m_closeBtn = new QPushButton(QStringLiteral("✕"), this);
    for (auto *b : {m_copyBtn, m_nextBtn, m_closeBtn}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(26);
        header->addWidget(b);
    }
    outer->addLayout(header);

    // 内容区
    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(false);
    m_browser->setFrameShape(QFrame::NoFrame);
    m_browser->verticalScrollBar()->setStyleSheet(QStringLiteral("QScrollBar:vertical{width:8px;}"));
    outer->addWidget(m_browser, 1);

    // 底部操作（朗读）
    auto *footer = new QHBoxLayout;
    m_speakerBtn = new QPushButton(QStringLiteral("🔊 朗读"), this);
    m_speakerBtn->setCursor(Qt::PointingHandCursor);
    m_speakerBtn->setFixedHeight(26);
    m_speakerBtn->setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:none;font-size:13px;color:#3E6FB0;}"));
    footer->addWidget(m_speakerBtn);
    footer->addStretch();
    outer->addLayout(footer);

    connect(m_copyBtn, &QPushButton::clicked, this, [this] {
        QGuiApplication::clipboard()->setText(m_core->buildFullText(m_item));
        m_copyBtn->setText(QStringLiteral("✓ 已复制"));
    });
    connect(m_nextBtn, &QPushButton::clicked, this, [this] { showNext(); });
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    connect(m_speakerBtn, &QPushButton::clicked, this, [this] {
        if (!m_speakText.isEmpty()) m_core->speak(m_speakText, m_speakLang);
    });

    applyTheme();
}

void LearnWindow::showNext()
{
    refresh(m_core->nextItem());
}

void LearnWindow::refresh(const QVariantMap &item)
{
    m_item = item;
    if (item.isEmpty()) {
        m_catLabel->setText(QStringLiteral("学习内容"));
        m_browser->setPlainText(QStringLiteral("还没有选择学习内容 🤔\n\n请 右键点击眼睛 → 学习内容\n勾选要学习的分类后再点眼睛"));
        m_speakerBtn->setVisible(false);
        return;
    }
    m_catLabel->setText(item.value(QStringLiteral("catName")).toString());
    const QString type = item.value(QStringLiteral("type")).toString();
    QString html;
    if (type == QLatin1String("poem")) {
        const QString t = item.value(QStringLiteral("t")).toString();
        const QString a = item.value(QStringLiteral("a")).toString();
        const QString sec = item.value(QStringLiteral("sec")).toString();
        const QString txt = item.value(QStringLiteral("txt")).toString();
        const QString trans = item.value(QStringLiteral("trans")).toString();
        const QString note = item.value(QStringLiteral("note")).toString();
        html = QStringLiteral("<div style='font-size:20px;font-weight:bold;'>%1</div>")
                   .arg(t.toHtmlEscaped());
        if (!a.isEmpty())
            html += QStringLiteral("<div style='font-size:12px;color:#8A919F;margin-top:2px;'>%1%2</div>")
                        .arg(a.toHtmlEscaped(), sec.isEmpty() ? QString() : QStringLiteral(" · ") + sec.toHtmlEscaped());
        html += QStringLiteral("<div style='font-size:15px;line-height:1.6;margin-top:8px;'>%1</div>")
                    .arg(txt.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        if (!trans.isEmpty())
            html += QStringLiteral("<hr><div style='font-size:12px;font-weight:bold;color:#3E6FB0;'>【译文】</div>"
                                   "<div style='font-size:13px;line-height:1.5;'>%1</div>")
                        .arg(trans.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        if (!note.isEmpty())
            html += QStringLiteral("<hr><div style='font-size:12px;font-weight:bold;color:#3E6FB0;'>【赏析·注释】</div>"
                                   "<div style='font-size:13px;color:#777;line-height:1.5;'>%1</div>")
                        .arg(note.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        m_speakText = txt; m_speakLang = QStringLiteral("zh");
    } else if (type == QLatin1String("wiki")) {
        const QString t = item.value(QStringLiteral("t")).toString();
        const QString d = item.value(QStringLiteral("d")).toString();
        html = QStringLiteral("<div style='font-size:20px;font-weight:bold;'>📖 %1</div>")
                   .arg(t.toHtmlEscaped());
        html += QStringLiteral("<hr><div style='font-size:14px;line-height:1.7;'>%1</div>")
                    .arg(d.toHtmlEscaped());
        html += QStringLiteral("<div style='font-size:11px;color:#999;margin-top:6px;'>（来源：维基百科）</div>");
        m_speakText = d; m_speakLang = QStringLiteral("zh");
    } else { // word
        const QString w = item.value(QStringLiteral("w")).toString();
        const QString p = item.value(QStringLiteral("p")).toString();
        const QString d = item.value(QStringLiteral("d")).toString();
        html = QStringLiteral("<div style='font-size:24px;font-weight:bold;'>%1</div>")
                   .arg(w.toHtmlEscaped());
        if (!p.isEmpty())
            html += QStringLiteral("<div style='font-size:13px;color:#8A919F;'>/%1/</div>").arg(p.toHtmlEscaped());
        html += QStringLiteral("<div style='font-size:14px;margin-top:4px;'>%1</div>").arg(d.toHtmlEscaped());
        const auto es = item.value(QStringLiteral("es")).toList();
        if (!es.isEmpty()) {
            html += QStringLiteral("<hr>");
            for (int i = 0; i < es.size(); ++i) {
                const auto e = es.at(i).toMap();
                html += QStringLiteral("<div style='font-size:12px;font-weight:bold;color:#3E6FB0;margin-top:6px;'>"
                                       "例句%1 🔊</div><div style='font-size:14px;font-style:italic;'>%2</div>")
                            .arg(i + 1)
                            .arg(e.value(QStringLiteral("e")).toString().toHtmlEscaped());
                if (e.contains(QStringLiteral("c")))
                    html += QStringLiteral("<div style='font-size:13px;color:#777;'>%1</div>")
                                .arg(e.value(QStringLiteral("c")).toString().toHtmlEscaped());
            }
        }
        m_speakText = w; m_speakLang = QStringLiteral("en");
    }
    m_browser->setHtml(html);
    m_speakerBtn->setVisible(true);
    adjustSize();
}

void LearnWindow::repositionNear(const QPoint &globalPos)
{
    QScreen *screen = QGuiApplication::screenAt(globalPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen->availableGeometry();
    const QSize sz = size();
    int x = globalPos.x() + 10;
    int y = globalPos.y() + 10;
    if (x + sz.width() > avail.right())  x = globalPos.x() - sz.width() - 10;
    if (y + sz.height() > avail.bottom()) y = globalPos.y() - sz.height() - 10;
    x = qBound(avail.left(), x, avail.right() - sz.width());
    y = qBound(avail.top(), y, avail.bottom() - sz.height());
    move(x, y);
}

void LearnWindow::applyTheme()
{
    const bool dark = systemDark();
    const QColor bg = dark ? QColor(35, 37, 41) : QColor(247, 249, 252);
    const QColor text = dark ? QColor(232, 234, 237) : QColor(31, 35, 41);
    const QColor sub = dark ? QColor(154, 160, 168) : QColor(138, 145, 159);
    m_browser->setStyleSheet(QStringLiteral(
        "QTextBrowser{background:transparent;color:%1;font-size:14px;}"
        "QScrollBar:vertical{background:transparent;width:8px;}"
        "QScrollBar::handle:vertical{background:%2;border-radius:4px;min-height:24px;}")
        .arg(text.name(), dark ? QStringLiteral("#4a4d55") : QStringLiteral("#c9cfd9")));
    m_browser->document()->setDefaultStyleSheet(QStringLiteral(
        "div{color:%1;}").arg(text.name()));
    const QString btnQss = QStringLiteral(
        "QPushButton{background:%1;border:1px solid %2;border-radius:12px;padding:2px 12px;font-size:12px;color:%3;}"
        "QPushButton:hover{background:%4;}")
        .arg(dark ? QStringLiteral("#34373d") : QStringLiteral("#e8edf3"),
             dark ? QStringLiteral("#3a3d42") : QStringLiteral("rgba(0,0,0,0.12)"),
             dark ? QStringLiteral("#5b8fd9") : QStringLiteral("#3e6fb0"),
             dark ? QStringLiteral("#41454d") : QStringLiteral("#dce4ee"));
    for (auto *b : {m_copyBtn, m_nextBtn, m_closeBtn}) b->setStyleSheet(btnQss);
    m_catLabel->setStyleSheet(QStringLiteral(
        "background:#3E6FB0;color:#fff;border-radius:11px;padding:3px 12px;font-size:12px;"));
}

void LearnWindow::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(), 14, 14);
    p.fillPath(path, systemDark() ? QColor(35, 37, 41) : QColor(247, 249, 252));
    Q_UNUSED(e);
}

void LearnWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void LearnWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging)
        move(e->globalPosition().toPoint() - m_dragOffset);
}


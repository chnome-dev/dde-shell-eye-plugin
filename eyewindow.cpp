// SPDX-License-Identifier: GPL-3.0-or-later
#include "eyewindow.h"
#include "learncore.h"
#include "learnwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QRandomGenerator>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <cmath>

static const char *kPetEmoji[] = {
    "🐭","🐮","🐯","🐰","🐲","🐍","🐴","🐑","🐵","🐔","🐶","🐷","🐟","🐱"
};

EyeWindow::EyeWindow(LearnCore *core)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_core(core)
    , m_learn(new LearnWindow(core))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(96, 96);

    QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
    m_eyeStyle = s.value(QStringLiteral("eyeStyle"), 0).toInt();
    m_petStyle = s.value(QStringLiteral("petStyle"), -1).toInt();
    m_posPref = s.value(QStringLiteral("posPref"), 3).toInt();

    loadThemes();
    buildMenu();

    m_tickTimer = new QTimer(this);
    connect(m_tickTimer, &QTimer::timeout, this, &EyeWindow::onTick);
    m_tickTimer->start(30);

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setSingleShot(true);
    connect(m_blinkTimer, &QTimer::timeout, this, [this] { onBlinkFrame(); });

    m_wanderTimer = new QTimer(this);
    m_wanderTimer->setSingleShot(true);
    connect(m_wanderTimer, &QTimer::timeout, this, [this] { onWanderFrame(); });

    refreshWindow();
    m_lastCursor = QCursor::pos();
}

void EyeWindow::loadThemes()
{
    m_themes = {
        {QStringLiteral("蓝色卡通"),  QColor(255,255,255), QColor(59,131,224),  QColor(20,30,50),  false,false,false,false,false, 0.42},
        {QStringLiteral("双眼睛 👀"), QColor(255,255,255), QColor(90,80,160),   QColor(25,20,45),  true, false,false,false,false, 0.40},
        {QStringLiteral("绿眸少女"),  QColor(252,248,242), QColor(66,150,88),   QColor(18,40,28),  false,false,false,false,true,  0.42},
        {QStringLiteral("粉眸少女"),  QColor(253,246,248), QColor(226,102,148), QColor(60,20,40),  false,false,false,false,true,  0.42},
        {QStringLiteral("浓睫大眼"),  QColor(255,255,255), QColor(120,90,60),   QColor(30,20,15),  false,false,false,false,true,  0.46},
        {QStringLiteral("恶魔红眼"),  QColor(140,10,10),   QColor(90,0,0),      QColor(0,0,0),     false,false,true, false,false, 0.46},
        {QStringLiteral("恐怖之眼"),  QColor(240,220,150), QColor(160,20,20),   QColor(0,0,0),     false,false,false,false,false, 0.44},
        {QStringLiteral("南瓜怪眼"),  QColor(255,150,40),  QColor(120,60,10),   QColor(30,15,0),   false,false,false,false,false, 0.42},
        {QStringLiteral("奇异双瞳"),  QColor(235,240,255), QColor(100,140,200), QColor(20,30,60),  false,false,false,true, false, 0.40},
        {QStringLiteral("猫瞳"),      QColor(210,245,180), QColor(80,140,60),   QColor(10,20,8),   false,true, false,false,false, 0.42},
        {QStringLiteral("哆啦A梦"),   QColor(80,160,255),  QColor(220,60,60),   QColor(10,10,10),  false,false,false,false,false, 0.50},
        {QStringLiteral("圆眼双瞳"),  QColor(255,255,255), QColor(120,90,160),  QColor(25,15,40),  false,false,false,true, false, 0.50},
        {QStringLiteral("蓝色卡通·双眼"), QColor(255,255,255), QColor(59,131,224),  QColor(20,30,50), true,false,false,false,false, 0.40},
        {QStringLiteral("绿眸·双眼"),     QColor(252,248,242), QColor(66,150,88),   QColor(18,40,28), true,false,false,false,true,  0.40},
        {QStringLiteral("粉眸·双眼"),     QColor(253,246,248), QColor(226,102,148), QColor(60,20,40), true,false,false,false,true,  0.40},
        {QStringLiteral("浓睫·双眼"),     QColor(255,255,255), QColor(120,90,60),   QColor(30,20,15), true,false,false,false,true,  0.42},
        {QStringLiteral("恶魔·双眼"),     QColor(140,10,10),   QColor(90,0,0),      QColor(0,0,0),    true,false,true, false,false, 0.42},
        {QStringLiteral("恐怖·双眼"),     QColor(240,220,150), QColor(160,20,20),   QColor(0,0,0),    true,false,false,false,false, 0.40},
        {QStringLiteral("南瓜·双眼"),     QColor(255,150,40),  QColor(120,60,10),   QColor(30,15,0),  true,false,false,false,false, 0.40},
        {QStringLiteral("猫瞳·双眼"),     QColor(210,245,180), QColor(80,140,60),   QColor(10,20,8),  true,true, false,false,false, 0.40},
    };
}

void EyeWindow::refreshWindow()
{
    const bool pet = m_petStyle >= 0 && m_petStyle < 14;
    setFixedSize(pet ? 72 : 96, pet ? 72 : 96);
    update();
}

QPointF EyeWindow::pupilOffset(const QPointF &local, const QRectF &eyeRect, qreal irisR) const
{
    const QPointF c = eyeRect.center();
    const QPointF d = local - c;
    const qreal maxDx = eyeRect.width() / 2.0 - irisR;
    const qreal maxDy = eyeRect.height() / 2.0 - irisR;
    const qreal len = std::hypot(d.x(), d.y());
    if (len < 3.0) return QPointF(0, 0);          // 中心死区
    // GEyes：方向向量缩放到椭圆边界
    const qreal k = std::min(1.0, std::min(maxDx / qMax(qAbs(d.x()), 0.001),
                                            maxDy / qMax(qAbs(d.y()), 0.001)));
    return d * k;
}

void EyeWindow::drawEye(QPainter &p, const QRectF &rect, const EyeTheme &t, qreal blink)
{
    // 眨眼：纵向压缩（以中心为原点）
    const qreal sy = 1.0 - 0.92 * blink;
    const QPointF c = rect.center();
    p.save();
    p.translate(c);
    p.scale(1.0, sy);
    p.translate(-c);

    p.setPen(Qt::NoPen);

    // 眼眶
    p.setBrush(t.sclera);
    p.drawEllipse(rect);

    if (t.demon) {
        // 恶魔红眼：血丝
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(255, 60, 60, 160));
        for (int i = -2; i <= 2; ++i) {
            p.drawLine(rect.center() + QPointF(i * 8, -rect.height() / 2),
                       rect.center() + QPointF(i * 3, rect.height() / 2));
        }
        p.setPen(Qt::NoPen);
    }

    // 瞳孔位置（跟随鼠标；空闲游走时叠加游走偏移）
    const QPointF mouseLocal = mapFromGlobal(QCursor::pos());
    const qreal irisR = rect.width() / 2.0 * t.irisScale;
    QPointF off = pupilOffset(mouseLocal, rect, irisR);
    if (m_wandering) {
        off += m_wanderPos;
        // 限制在椭圆内
        const qreal maxDx = rect.width() / 2.0 - irisR;
        const qreal maxDy = rect.height() / 2.0 - irisR;
        const qreal k = std::min(1.0, std::min(maxDx / qMax(qAbs(off.x()), 0.001),
                                               maxDy / qMax(qAbs(off.y()), 0.001)));
        off *= k;
    }

    // 虹膜
    const QPointF irisC = rect.center() + off;
    const QRectF irisRect(irisC.x() - irisR, irisC.y() - irisR, irisR * 2, irisR * 2);
    p.setBrush(t.iris);
    p.drawEllipse(irisRect);

    // 瞳孔
    const qreal pupR = irisR * 0.55;
    if (t.catPupil) {
        // 猫瞳：竖条
        p.setBrush(t.pupil);
        p.drawRoundedRect(QRectF(irisC.x() - pupR * 0.45, irisC.y() - pupR * 1.4,
                                 pupR * 0.9, pupR * 2.8), pupR * 0.4, pupR * 0.4);
    } else {
        p.setBrush(t.pupil);
        p.drawEllipse(QRectF(irisC.x() - pupR, irisC.y() - pupR, pupR * 2, pupR * 2));
        if (t.twin) {
            // 双瞳：右侧再画一个
            p.drawEllipse(QRectF(irisC.x() - pupR + pupR * 1.1, irisC.y() - pupR,
                                 pupR * 1.6, pupR * 1.6));
        }
    }

    // 高光
    p.setBrush(QColor(255, 255, 255, 200));
    p.drawEllipse(QPointF(irisC.x() - irisR * 0.35, irisC.y() - irisR * 0.4),
                  irisR * 0.22, irisR * 0.22);

    // 睫毛
    if (t.lash) {
        p.setPen(QPen(QColor(60, 40, 30), rect.width() * 0.035));
        for (int i = -2; i <= 2; ++i) {
            const qreal a = i * 0.32 - 0.5;
            const QPointF base(rect.center().x() + a * rect.width() * 0.28,
                               rect.top() + rect.height() * 0.06);
            p.drawLine(base, base + QPointF(a * rect.width() * 0.1, -rect.height() * 0.12));
        }
        p.setPen(Qt::NoPen);
    }
    p.restore();
}

void EyeWindow::drawPet(QPainter &p)
{
    const QString emoji = QString::fromUtf8(kPetEmoji[m_petStyle]);
    QFont f = font();
    f.setPixelSize(56);
    p.setFont(f);
    // 轻微上下浮动
    const qreal dy = std::sin(m_petPhase) * 3.0;
    p.drawText(rect().adjusted(0, int(dy), 0, int(dy)), Qt::AlignCenter, emoji);
}

void EyeWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (m_petStyle >= 0 && m_petStyle < 14) {
        drawPet(p);
        return;
    }
    const EyeTheme &t = m_themes.at(qBound(0, m_eyeStyle, m_themes.size() - 1));
    if (t.doubleEye) {
        // 双眼并排（👀 风格）
        const qreal w = width() * 0.40, h = height() * 0.62;
        const qreal y = (height() - h) / 2;
        const qreal x1 = width() * 0.06, x2 = width() * 0.54;
        drawEye(p, QRectF(x1, y, w, h), t, m_blink);
        drawEye(p, QRectF(x2, y, w, h), t, m_blink);
    } else {
        const qreal w = width() * 0.82, h = height() * 0.86;
        drawEye(p, QRectF((width() - w) / 2, (height() - h) / 2, w, h), t, m_blink);
    }
}

void EyeWindow::onTick()
{
    const QPoint cur = QCursor::pos();
    if (cur != m_lastCursor) {
        m_lastCursor = cur;
        m_lastMoveMs = QDateTime::currentMSecsSinceEpoch();
        m_wandering = false;
        m_wanderTimer->stop();
    }
    // 空闲 4 秒 -> 开始东张西望
    if (!m_wandering && !m_petStyle &&
        QDateTime::currentMSecsSinceEpoch() - m_lastMoveMs > 4000) {
        m_wandering = true;
        m_wanderTarget = QPointF((QRandomGenerator::global()->bounded(200) - 100) / 10.0,
                                 (QRandomGenerator::global()->bounded(200) - 100) / 10.0);
        m_wanderPos = QPointF(0, 0);
        m_wanderTimer->start(120);
    }
    if (m_petStyle >= 0 && m_petStyle < 14) {
        m_petPhase += 0.06;
        update();
    } else {
        update();
    }
}

void EyeWindow::onBlinkFrame()
{
    // 眨眼动画：约 12 帧 0->1->0
    static int frame = 0;
    static bool up = true;
    if (frame == 0) up = true;
    m_blink = up ? frame / 6.0 : (12 - frame) / 6.0;
    frame += 1;
    update();
    if (frame <= 12) {
        QTimer::singleShot(24, this, [this] { onBlinkFrame(); });
    } else {
        frame = 0;
        m_blink = 0.0;
        // 随机 2.2~7 秒后再眨
        m_blinkTimer->start(QRandomGenerator::global()->bounded(2200, 7000));
    }
}

void EyeWindow::onWanderFrame()
{
    if (!m_wandering) return;
    // 平滑靠近目标
    m_wanderPos += (m_wanderTarget - m_wanderPos) * 0.15;
    if ((m_wanderTarget - m_wanderPos).manhattanLength() < 0.5) {
        // 换一个新目标，10% 概率结束游走
        if (QRandomGenerator::global()->bounded(10) == 0) {
            m_wandering = false;
            m_wanderPos = QPointF(0, 0);
            return;
        }
        m_wanderTarget = QPointF((QRandomGenerator::global()->bounded(200) - 100) / 10.0,
                                 (QRandomGenerator::global()->bounded(200) - 100) / 10.0);
    }
    update();
    m_wanderTimer->start(120);
}

void EyeWindow::showAtPreferred()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    const QRect avail = screen->availableGeometry();
    const QSize sz = size();
    QPoint pos;
    switch (m_posPref) {
    case 0: pos = avail.topLeft() + QPoint(20, 20); break;
    case 1: pos = avail.topRight() - QPoint(sz.width() + 20, -20); break;
    case 2: pos = avail.bottomLeft() + QPoint(20, -20 - sz.height()); break;
    case 3: pos = avail.bottomRight() - QPoint(sz.width() + 20, sz.height() + 20); break;
    default: {
        QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
        pos = s.value(QStringLiteral("freePos"), QPoint(200, 200)).toPoint();
        break;
    }
    }
    move(pos);
    show();
}

void EyeWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressPos = e->globalPosition().toPoint();
        m_pressed = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        m_dragging = false;
    }
}

void EyeWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_pressed) {
        const bool wasDrag = m_dragging;
        m_pressed = false;
        m_dragging = false;
        if (!wasDrag && (e->globalPosition().toPoint() - m_pressPos).manhattanLength() < 5) {
            // 单击：弹出学习内容
            m_learn->showNext();
            m_learn->repositionNear(e->globalPosition().toPoint());
            m_learn->show();
            m_learn->raise();
            m_learn->activateWindow();
        }
    }
}

void EyeWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_pressed && (e->buttons() & Qt::LeftButton)) {
        if ((e->globalPosition().toPoint() - m_pressPos).manhattanLength() > 5)
            m_dragging = true;
        if (m_dragging) {
            m_posPref = 4; // 自由位置
            QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
            s.setValue(QStringLiteral("posPref"), 4);
            s.setValue(QStringLiteral("freePos"), e->globalPosition().toPoint() - m_dragOffset);
            move(e->globalPosition().toPoint() - m_dragOffset);
        }
    }
}

void EyeWindow::contextMenuEvent(QContextMenuEvent *e)
{
    if (m_menu) m_menu->popup(e->globalPos());
}

void EyeWindow::enterEvent(QEnterEvent *)
{
    m_lastMoveMs = QDateTime::currentMSecsSinceEpoch();
}

void EyeWindow::leaveEvent(QEvent *)
{
}

void EyeWindow::updatePetAnim()
{
    update();
}

void EyeWindow::buildMenu()
{
    m_menu = new QMenu(this);
    m_menu->setTitle(QStringLiteral("设置"));
    // 根菜单（设置）高度固定为 225px
    // 二级菜单（眼睛样式/宠物样式/学习内容/显示位置）由 QMenu 原生按内容自动调整高度
    m_menu->setFixedHeight(225);

    // ---- 眼睛样式 ----
    auto *eyeMenu = m_menu->addMenu(QStringLiteral("眼睛样式"));
    auto *eyeGroup = new QActionGroup(eyeMenu);
    for (int i = 0; i < m_themes.size(); ++i) {
        auto *act = eyeMenu->addAction(m_themes.at(i).name);
        act->setCheckable(true);
        act->setChecked(i == m_eyeStyle);
        act->setData(i);
        eyeGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, i] {
            m_eyeStyle = i;
            m_petStyle = -1;
            QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
            s.setValue(QStringLiteral("eyeStyle"), i);
            s.setValue(QStringLiteral("petStyle"), -1);
            refreshWindow();
        });
    }

    // ---- 宠物样式 ----
    auto *petMenu = m_menu->addMenu(QStringLiteral("宠物样式"));
    auto *petGroup = new QActionGroup(petMenu);
    const char *petNames[] = {"🐭 鼠","🐮 牛","🐯 虎","🐰 兔","🐲 龙","🐍 蛇","🐴 马",
                              "🐑 羊","🐵 猴","🐔 鸡","🐶 狗","🐷 猪","🐟 鱼","🐱 猫"};
    for (int i = 0; i < 14; ++i) {
        auto *act = petMenu->addAction(QString::fromUtf8(petNames[i]));
        act->setCheckable(true);
        act->setChecked(i == m_petStyle);
        act->setData(i);
        petGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, i] {
            m_petStyle = i;
            QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
            s.setValue(QStringLiteral("petStyle"), i);
            refreshWindow();
        });
    }
    petMenu->addSeparator();
    auto *noPet = petMenu->addAction(QStringLiteral("🙈 关闭宠物（眼睛模式）"));
    noPet->setCheckable(true);
    noPet->setChecked(m_petStyle < 0);
    petGroup->addAction(noPet);
    connect(noPet, &QAction::triggered, this, [this] {
        m_petStyle = -1;
        QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
        s.setValue(QStringLiteral("petStyle"), -1);
        refreshWindow();
    });

    // ---- 学习内容 ----
    auto *learnMenu = m_menu->addMenu(QStringLiteral("学习内容"));
    auto addCat = [this, learnMenu](const QString &label, int idx) {
        auto *act = learnMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_core->categoryEnabled(idx));
        connect(act, &QAction::toggled, this, [this, idx](bool on) {
            m_core->setCategoryEnabled(idx, on);
        });
        return act;
    };
    auto *enH = learnMenu->addAction(QStringLiteral("英文学习"));
    enH->setEnabled(false);
    for (int i = 3; i <= 8; ++i) addCat(m_core->categoryName(i), i);
    auto *poH = learnMenu->addAction(QStringLiteral("诗歌学习"));
    poH->setEnabled(false);
    for (int i = 0; i <= 2; ++i) addCat(m_core->categoryName(i), i);
    auto *bkH = learnMenu->addAction(QStringLiteral("百科学习"));
    bkH->setEnabled(false);
    addCat(m_core->categoryName(9), 9);

    // ---- 显示位置 ----
    auto *posMenu = m_menu->addMenu(QStringLiteral("显示位置"));
    auto *posGroup = new QActionGroup(posMenu);
    const char *posNames[] = {"左上角", "右上角", "左下角", "右下角", "自由拖动"};
    for (int i = 0; i < 5; ++i) {
        auto *act = posMenu->addAction(QString::fromUtf8(posNames[i]));
        act->setCheckable(true);
        act->setChecked(i == m_posPref);
        posGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, i] {
            m_posPref = i;
            QSettings s(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"));
            s.setValue(QStringLiteral("posPref"), i);
            showAtPreferred();
        });
    }

    // ---- 关于 ----
    m_menu->addSeparator();
    m_menu->addAction(QStringLiteral("退出"), this, [] { QApplication::quit(); });

    m_menu->addAction(QStringLiteral("关于"), this, [this] {
        QMessageBox::about(this, QStringLiteral("关于 卡通眼珠"),
            QStringLiteral("<b>卡通眼珠 v1.0 (Windows)</b><br>"
                           "· 20 套眼睛主题 / 宠物模式（十二生肖·鱼·猫）<br>"
                           "· 左键单击弹出学习内容（唐诗宋词/诗经/英语词汇/维基百科精选词条）<br>"
                           "· 瞳孔跟随参照 GEyes（gnome-applets）算法<br>"
                           "· TTS 使用 Windows 系统语音<br>"
                           "· 数据随程序内置（data/*.json）"));
    });
}

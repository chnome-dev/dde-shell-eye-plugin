// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// deepin V20 (dde-dock) 版卡通眼珠插件 —— 绘制层实现。
// 主题表 / 宠物表 / 绘制逻辑均逐项对齐 V25 版 main.qml。

#include "eyeartist.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

// ---------------------------------------------------------------------------
// PetOp 构造
// ---------------------------------------------------------------------------
PetOp PetOp::rect(qreal x, qreal y, qreal w, qreal h, qreal radius,
                  const QString &color, qreal borderW, const QString &borderColor,
                  qreal rotation, bool absH, bool radiusAbs,
                  int anim, qreal animA, qreal animB, int animMs)
{
    PetOp o;
    o.kind = Rect;
    o.x = x;
    o.y = y;
    o.w = w;
    o.h = h;
    o.radius = radius;
    o.radiusAbs = radiusAbs;
    o.rotation = rotation;
    o.absH = absH;
    o.color = QColor(color);
    o.borderW = borderW;
    if (borderW > 0 && !borderColor.isEmpty())
        o.border = QColor(borderColor);
    o.anim = anim;
    o.animA = animA;
    o.animB = animB;
    o.animMs = animMs;
    return o;
}

PetOp PetOp::eye(qreal x, qreal y, qreal s)
{
    PetOp o;
    o.kind = Eye;
    o.x = x;
    o.y = y;
    o.s = s;
    return o;
}

PetOp PetOp::label(qreal x, qreal y, qreal fontRatio, const QString &t, const QString &color)
{
    PetOp o;
    o.kind = Text;
    o.x = x;
    o.y = y;
    o.fontRatio = fontRatio;
    o.text = t;
    o.color = QColor(color);
    return o;
}

// ---------------------------------------------------------------------------
// 眼睛主题表（20 条，顺序 = 菜单顺序 = eyeStyle）
// ---------------------------------------------------------------------------
namespace {

EyeTheme mk(const QString &cn, const QString &en, bool dbl,
            const QString &sclera, const QString &border, const QString &iris,
            const QString &pupil, const QString &hl,
            bool lid, const QString &lidColor, bool lashes, bool slit,
            bool round = false, qreal irisScale = 0.60, qreal pupilScale = 0.52)
{
    EyeTheme t;
    t.name = en.isEmpty() ? cn : QStringLiteral("%1 (%2)").arg(cn, en);
    t.dbl = dbl;
    t.sclera = QColor(sclera);
    t.border = QColor(border);
    t.iris = QColor(iris);
    t.pupil = QColor(pupil);
    t.hl = QColor(hl);
    t.lid = lid;
    t.lidColor = QColor(lidColor);
    t.lashes = lashes;
    t.slit = slit;
    t.round = round;
    t.irisScale = irisScale;
    t.pupilScale = pupilScale;
    return t;
}

QVector<EyeTheme> buildThemes()
{
    QVector<EyeTheme> v;
    //                      中文名        英文名(菜单后缀)             双眼   眼白       眼眶       虹膜       瞳孔       高光       眼睑  眼睑色      睫毛   竖瞳
    v << mk(QStringLiteral("蓝色卡通"),   QStringLiteral("Default"),        false, "#FDFDFD", "#262626", "#3E6FB0", "#151515", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("双眼睛"),     QStringLiteral("Bizarre"),        true,  "#FFFFFF", "#2B2B2B", "#33231A", "#151515", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("绿眸少女"),   QStringLiteral("Green-EyedGirl"), false, "#F0F8F0", "#2E7D32", "#43A047", "#0B3D0B", "#FFFFFF", true,  "#FFE0B2", false, false);
    v << mk(QStringLiteral("粉眸少女"),   QStringLiteral("Pink-EyedGirl"),  false, "#FFF0F5", "#D81B60", "#F06292", "#7B1E3C", "#FFFFFF", true,  "#FFD9E8", false, false);
    v << mk(QStringLiteral("浓睫大眼"),   QStringLiteral("EyelashLarge"),   false, "#FFFFFF", "#1A1A1A", "#1976D2", "#0D1B2A", "#FFFFFF", true,  "#FFE0B2", true,  false);
    v << mk(QStringLiteral("恶魔红眼"),   QStringLiteral("Bloodshot"),      false, "#FDE8E8", "#8E0000", "#C62828", "#3E0000", "#FFD0D0", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("恐怖之眼"),   QStringLiteral("Horrid"),         false, "#141414", "#000000", "#FFD600", "#1A1A00", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("南瓜怪眼"),   QStringLiteral("PumpkinMonster"), false, "#FFF3E0", "#B4540A", "#E67E22", "#4A2400", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("奇异双瞳"),   QStringLiteral("Bizarre-2"),      true,  "#EEEEEE", "#424242", "#7E57C2", "#1A1033", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("猫瞳"),       QStringLiteral("Cat"),            false, "#F1F8E9", "#33691E", "#9CCC65", "#1B3A0B", "#FFFFFF", false, "#F2C9A0", false, true);
    v << mk(QStringLiteral("哆啦A梦"),    QString(),                        true,  "#FFFFFF", "#222222", "#1E1E1E", "#000000", "#FFFFFF", false, "#F2C9A0", false, false, true, 0.55, 0.80);
    v << mk(QStringLiteral("圆眼双瞳"),   QString(),                        true,  "#FFFFFF", "#2B2B2B", "#1976D2", "#0D1B2A", "#FFFFFF", false, "#F2C9A0", false, false, true, 0.55, 0.50);
    v << mk(QStringLiteral("蓝色卡通·双眼"), QString(),                     true,  "#FDFDFD", "#262626", "#3E6FB0", "#151515", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("绿眸少女·双眼"), QString(),                     true,  "#F0F8F0", "#2E7D32", "#43A047", "#0B3D0B", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("粉眸少女·双眼"), QString(),                     true,  "#FFF0F5", "#D81B60", "#F06292", "#7B1E3C", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("浓睫大眼·双眼"), QString(),                     true,  "#FFFFFF", "#1A1A1A", "#1976D2", "#0D1B2A", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("恶魔红眼·双眼"), QString(),                     true,  "#FDE8E8", "#8E0000", "#C62828", "#3E0000", "#FFD0D0", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("恐怖之眼·双眼"), QString(),                     true,  "#141414", "#000000", "#FFD600", "#1A1A00", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("南瓜怪眼·双眼"), QString(),                     true,  "#FFF3E0", "#B4540A", "#E67E22", "#4A2400", "#FFFFFF", false, "#F2C9A0", false, false);
    v << mk(QStringLiteral("猫瞳·双眼"),   QString(),                       true,  "#F1F8E9", "#33691E", "#9CCC65", "#1B3A0B", "#FFFFFF", false, "#F2C9A0", false, false);
    return v;
}

// ---------------------------------------------------------------------------
// 宠物表（14 只，顺序 = 菜单顺序 = petStyle）
// 坐标全部照搬 main.qml，R(x,y,w,h,radius,color[,borderW,borderColor][,rotation])
// ---------------------------------------------------------------------------
namespace pet {

// 摆尾/摇耳等往复旋转动画
constexpr int kWag = 1;
// 水平游动
constexpr int kSwayX = 2;

PetDef makeMouse()
{
    PetDef d;
    d.name = QStringLiteral("鼠 🐭");
    d.ops << PetOp::rect(.06, .02, .30, .30, .15, "#C9A9A6", 1, "#7A5C58")
          << PetOp::rect(.64, .02, .30, .30, .15, "#C9A9A6", 1, "#7A5C58")
          << PetOp::rect(.14, .14, .72, .72, .36, "#D8BCB9", 1.5, "#5C4644")
          << PetOp::eye(.34, .34, .11)
          << PetOp::eye(.55, .34, .11)
          << PetOp::rect(.42, .56, .16, .10, .05, "#F49CA0")
          << PetOp::rect(.40, .62, .05, .10, .03, "#FFFFFF")
          << PetOp::rect(.55, .62, .05, .10, .03, "#FFFFFF")
          << PetOp::rect(.08, .50, .16, 1, 0, "#8A6A55", 0, QString(), 10, true)
          << PetOp::rect(.76, .50, .16, 1, 0, "#8A6A55", 0, QString(), -10, true);
    return d;
}

PetDef makeOx()
{
    PetDef d;
    d.name = QStringLiteral("牛 🐮");
    d.ops << PetOp::rect(.18, .04, .22, .14, .07, "#8A6B3F")
          << PetOp::rect(.60, .04, .22, .14, .07, "#8A6B3F")
          << PetOp::rect(.14, .16, .72, .70, .34, "#A97F4F", 1.5, "#4E3A22")
          << PetOp::eye(.35, .34, .10)
          << PetOp::eye(.55, .34, .10)
          << PetOp::rect(.41, .58, .18, .12, .04, "#6B4A26")
          << PetOp::rect(.43, .58, .03, .06, 1, "#3A2712", 0, QString(), 0, false, true)
          << PetOp::rect(.54, .58, .03, .06, 1, "#3A2712", 0, QString(), 0, false, true);
    return d;
}

PetDef makeTiger()
{
    PetDef d;
    d.name = QStringLiteral("虎 🐯");
    d.ops << PetOp::rect(.18, .04, .20, .14, .07, "#D98E32")
          << PetOp::rect(.62, .04, .20, .14, .07, "#D98E32")
          << PetOp::rect(.14, .14, .72, .72, .36, "#F0A23C", 1.5, "#4E3A22")
          << PetOp::rect(.30, .20, .40, .05, 2, "#4E3A22", 0, QString(), -8, false, true)
          << PetOp::rect(.30, .25, .40, .05, 2, "#4E3A22", 0, QString(), 8, false, true)
          << PetOp::eye(.33, .34, .11)
          << PetOp::eye(.56, .34, .11)
          << PetOp::rect(.36, .56, .14, .10, .03, "#F7D9A8")
          << PetOp::rect(.52, .56, .12, .08, .03, "#F7D9A8")
          << PetOp::label(.43, .21, .13, QStringLiteral("王"), "#4E3A22");
    return d;
}

PetDef makeRabbit()
{
    PetDef d;
    d.name = QStringLiteral("兔 🐰");
    d.ops << PetOp::rect(.30, .00, .16, .42, .08, "#F4E9DC", 1, "#9A8A78", 0, false, false, kWag, 8, -8, 600)
          << PetOp::rect(.54, .00, .16, .42, .08, "#F4E9DC", 1, "#9A8A78")
          << PetOp::rect(.14, .20, .72, .66, .33, "#FBF4EC", 1.5, "#9A8A78")
          << PetOp::eye(.35, .36, .10)
          << PetOp::eye(.55, .36, .10)
          << PetOp::rect(.45, .52, .10, .07, .05, "#F49CA0");
    return d;
}

PetDef makeDragon()
{
    PetDef d;
    d.name = QStringLiteral("龙 🐲");
    d.ops << PetOp::rect(.29, .00, .07, .26, 3, "#5A7A3A", 0, QString(), -10, false, true)
          << PetOp::rect(.27, .00, .06, .12, 3, "#5A7A3A", 0, QString(), -45, false, true)
          << PetOp::rect(.64, .00, .07, .26, 3, "#5A7A3A", 0, QString(), 10, false, true)
          << PetOp::rect(.67, .00, .06, .12, 3, "#5A7A3A", 0, QString(), 45, false, true)
          << PetOp::rect(.14, .14, .72, .72, .36, "#8FC65B", 1.5, "#3E5A28")
          << PetOp::rect(.24, .26, .52, .05, 2, "#6FA640", 0, QString(), 0, false, true)
          << PetOp::rect(.24, .72, .52, .05, 2, "#6FA640", 0, QString(), 0, false, true)
          << PetOp::rect(.26, .27, .18, .06, 3, "#3E5A28", 0, QString(), -15, false, true)
          << PetOp::rect(.56, .27, .18, .06, 3, "#3E5A28", 0, QString(), 15, false, true)
          << PetOp::eye(.32, .36, .11)
          << PetOp::eye(.57, .36, .11)
          << PetOp::rect(.43, .52, .14, .10, .05, "#D9534F")
          << PetOp::rect(.465, .46, .07, .07, .035, "#FFD700")
          << PetOp::rect(.32, .61, .36, 1.5, 0, "#3E5A28", 0, QString(), -16, true)
          << PetOp::rect(.32, .61, .36, 1.5, 0, "#3E5A28", 0, QString(), 16, true);
    return d;
}

PetDef makeSnake()
{
    PetDef d;
    d.name = QStringLiteral("蛇 🐍");
    d.bodyAnim = 1;
    d.bodyA = 6;
    d.bodyMs = 700;
    d.ops << PetOp::rect(.20, .26, .60, .30, .15, "#7DBE5A", 1.5, "#3E5A28")
          << PetOp::eye(.30, .32, .09)
          << PetOp::eye(.46, .32, .09)
          << PetOp::rect(.48, .46, .05, .14, .03, "#D9534F", 0, QString(), -25)
          << PetOp::rect(.48, .46, .05, .14, .03, "#D9534F", 0, QString(), 25);
    return d;
}

PetDef makeHorse()
{
    PetDef d;
    d.name = QStringLiteral("马 🐴");
    d.ops << PetOp::rect(.26, .04, .12, .34, .06, "#8A5A2B")
          << PetOp::rect(.62, .04, .12, .34, .06, "#8A5A2B")
          << PetOp::rect(.16, .22, .68, .62, .30, "#B57B3C", 1.5, "#4E3A22")
          << PetOp::eye(.36, .36, .10)
          << PetOp::eye(.56, .36, .10)
          << PetOp::rect(.45, .56, .10, .08, .03, "#5C3A1C");
    return d;
}

PetDef makeGoat()
{
    PetDef d;
    d.name = QStringLiteral("羊 🐑");
    d.ops << PetOp::rect(.20, .02, .16, .20, .08, "#D8C8A8", 0, QString(), -20)
          << PetOp::rect(.64, .02, .16, .20, .08, "#D8C8A8", 0, QString(), 20)
          << PetOp::rect(.14, .16, .72, .70, .34, "#F0E6D2", 1.5, "#8A7A60")
          << PetOp::rect(.41, .42, .18, .16, .09, "#8A7A60")
          << PetOp::eye(.32, .32, .09)
          << PetOp::eye(.58, .32, .09)
          << PetOp::rect(.10, .24, .14, .14, .07, "#FFFFFF")
          << PetOp::rect(.76, .24, .14, .14, .07, "#FFFFFF")
          << PetOp::rect(.42, .04, .16, .14, .07, "#FFFFFF");
    return d;
}

PetDef makeMonkey()
{
    PetDef d;
    d.name = QStringLiteral("猴 🐵");
    d.ops << PetOp::rect(.06, .10, .26, .26, .13, "#9A6B3F")
          << PetOp::rect(.68, .10, .26, .26, .13, "#9A6B3F")
          << PetOp::rect(.15, .15, .70, .70, .33, "#B07B45", 1.5, "#4E3A22")
          << PetOp::rect(.27, .22, .46, .42, .20, "#E8C9A0")
          << PetOp::eye(.33, .34, .10)
          << PetOp::eye(.57, .34, .10)
          << PetOp::rect(.47, .52, .06, .05, .03, "#8A6A45");
    return d;
}

PetDef makeRooster()
{
    PetDef d;
    d.name = QStringLiteral("鸡 🐔");
    d.ops << PetOp::rect(.35, .02, .30, .14, .04, "#D94040")
          << PetOp::rect(.24, .08, .20, .10, .05, "#D94040", 0, QString(), -25)
          << PetOp::rect(.56, .08, .20, .10, .05, "#D94040", 0, QString(), 25)
          << PetOp::rect(.14, .20, .72, .66, .33, "#F2C94C", 1.5, "#8A6A1C")
          << PetOp::eye(.35, .36, .10)
          << PetOp::eye(.55, .36, .10)
          << PetOp::rect(.45, .56, .10, .08, .03, "#E07A2A")
          << PetOp::rect(.24, .46, .06, .06, .03, "#FFFFFF")
          << PetOp::rect(.70, .46, .06, .06, .03, "#FFFFFF");
    return d;
}

PetDef makeDog()
{
    PetDef d;
    d.name = QStringLiteral("狗 🐶");
    d.ops << PetOp::rect(.16, .12, .20, .34, .10, "#A97A45")
          << PetOp::rect(.64, .12, .20, .34, .10, "#A97A45")
          << PetOp::rect(.14, .20, .72, .66, .33, "#C99A5E", 1.5, "#4E3A22")
          << PetOp::eye(.34, .36, .10)
          << PetOp::eye(.56, .36, .10)
          << PetOp::rect(.43, .56, .14, .10, .05, "#3A2712")
          << PetOp::rect(.46, .62, .08, .06, .03, "#F49CA0")
          << PetOp::rect(.80, .72, .10, .18, .05, "#A97A45", 0, QString(), 0, false, false, kWag, 20, -10, 500);
    return d;
}

PetDef makePig()
{
    PetDef d;
    d.name = QStringLiteral("猪 🐷");
    d.ops << PetOp::rect(.20, .10, .16, .12, .06, "#F0A0B0")
          << PetOp::rect(.64, .10, .16, .12, .06, "#F0A0B0")
          << PetOp::rect(.14, .14, .72, .70, .35, "#F8B4C0", 1.5, "#9A5A6A")
          << PetOp::eye(.34, .34, .10)
          << PetOp::eye(.56, .34, .10)
          << PetOp::rect(.39, .54, .22, .16, .08, "#F49CA0")
          << PetOp::rect(.45, .56, .04, .06, 2, "#9A5A6A", 0, QString(), 0, false, true)
          << PetOp::rect(.51, .56, .04, .06, 2, "#9A5A6A", 0, QString(), 0, false, true)
          << PetOp::rect(.24, .48, .08, .06, .04, "#F49CA0")
          << PetOp::rect(.68, .48, .08, .06, .04, "#F49CA0");
    return d;
}

PetDef makeFish()
{
    PetDef d;
    d.name = QStringLiteral("鱼 🐟");
    d.ops << PetOp::rect(.16, .28, .60, .42, .20, "#5BA8E8", 1.5, "#2A5A8A", 0, false, false, kSwayX, 5)
          << PetOp::rect(.68, .34, .20, .24, .05, "#4A8CC8", 0, QString(), 0, false, false, kWag, 25, -25, 400)
          << PetOp::rect(.36, .20, .14, .12, .06, "#4A8CC8")
          << PetOp::eye(.26, .36, .09)
          << PetOp::rect(.10, .10, .06, .06, .03, "#BBD8F0", 0.5, "#7AA0C0")
          << PetOp::rect(.06, .18, .04, .04, .02, "#BBD8F0");
    return d;
}

PetDef makeCat()
{
    PetDef d;
    d.name = QStringLiteral("猫 🐱");
    d.ops << PetOp::rect(.14, .02, .18, .24, 0, "#E8833A", 0, QString(), -16)
          << PetOp::rect(.17, .06, .08, .12, 0, "#F8C8B0", 0, QString(), -16)
          << PetOp::rect(.68, .02, .18, .24, 0, "#E8833A", 0, QString(), 16)
          << PetOp::rect(.75, .06, .08, .12, 0, "#F8C8B0", 0, QString(), 16)
          << PetOp::rect(.15, .16, .70, .66, .33, "#F0A050", 1.5, "#8A4A20")
          << PetOp::rect(.37, .22, .26, .07, 3, "#C96A28", 0, QString(), -6, false, true)
          << PetOp::eye(.31, .34, .12)
          << PetOp::eye(.57, .34, .12)
          << PetOp::rect(.455, .52, .09, .07, .035, "#F06070")
          << PetOp::rect(.44, .57, .06, .05, .03, "#8A4A20")
          << PetOp::rect(.50, .57, .06, .05, .03, "#8A4A20")
          << PetOp::rect(.12, .50, .20, 1.5, 0, "#8A6A4A", 0, QString(), 12, true)
          << PetOp::rect(.68, .50, .20, 1.5, 0, "#8A6A4A", 0, QString(), -12, true)
          << PetOp::rect(.13, .60, .18, 1.5, 0, "#8A6A4A", 0, QString(), -10, true)
          << PetOp::rect(.69, .60, .18, 1.5, 0, "#8A6A4A", 0, QString(), 10, true)
          << PetOp::rect(.80, .68, .09, .24, .045, "#E8833A", 0, QString(), 0, false, false, kWag, 25, -15, 600);
    return d;
}

} // namespace pet

QVector<PetDef> buildPets()
{
    QVector<PetDef> v;
    v << pet::makeMouse() << pet::makeOx() << pet::makeTiger() << pet::makeRabbit()
      << pet::makeDragon() << pet::makeSnake() << pet::makeHorse() << pet::makeGoat()
      << pet::makeMonkey() << pet::makeRooster() << pet::makeDog() << pet::makePig()
      << pet::makeFish() << pet::makeCat();
    return v;
}

// ---------------------------------------------------------------------------
// 绘制小工具
// ---------------------------------------------------------------------------
QPainterPath roundedPath(const QRectF &r, qreal radius)
{
    const qreal lim = qMin(r.width(), r.height()) / 2.0;
    const qreal rad = qBound(0.0, radius, lim);
    QPainterPath path;
    if (rad <= 0.01) {
        path.addRect(r);
    } else {
        path.addRoundedRect(r, rad, rad);
    }
    return path;
}

void fillRounded(QPainter *p, const QRectF &r, qreal radius, const QColor &color,
                 qreal borderW = 0, const QColor &borderColor = QColor())
{
    if (!color.isValid() || color.alpha() == 0)
        return;
    if (borderW > 0 && borderColor.isValid()) {
        // Qt Quick 的 border 画在矩形内侧：此处把描边路径整体内缩半个线宽
        const QRectF inner = r.adjusted(borderW / 2, borderW / 2, -borderW / 2, -borderW / 2);
        QPen pen(borderColor);
        pen.setWidthF(borderW);
        pen.setJoinStyle(Qt::RoundJoin);
        p->setPen(pen);
        p->setBrush(color);
        p->drawPath(roundedPath(inner, radius - borderW / 2));
    } else {
        p->setPen(Qt::NoPen);
        p->setBrush(color);
        p->drawPath(roundedPath(r, radius));
    }
}

// 正数取模。不用 std::fmod —— 在高版本 glibc 上会绑定 fmod@GLIBC_2.38，
// 导致 deepin V20（glibc 2.28）无法加载本插件。
qreal modPos(qreal v, qreal m)
{
    if (m <= 0)
        return 0;
    return v - qFloor(v / m) * m;
}

} // namespace

const QVector<EyeTheme> &eyeThemes()
{
    static const QVector<EyeTheme> v = buildThemes();
    return v;
}

const QVector<PetDef> &petDefs()
{
    static const QVector<PetDef> v = buildPets();
    return v;
}

int eyeThemeCount() { return eyeThemes().size(); }
int petCount() { return petDefs().size(); }

// ---------------------------------------------------------------------------
// GEyes 瞳孔偏移（与 main.qml computePupilOffset 完全一致）
// ---------------------------------------------------------------------------
QPointF computePupilOffset(qreal localX, qreal localY, qreal cx, qreal cy,
                           qreal eyeRx, qreal eyeRy, qreal pupilR, qreal wall)
{
    const qreal nx = localX - cx;
    const qreal ny = localY - cy;
    const qreal h = qSqrt(nx * nx + ny * ny);
    const qreal deadZone = qMin(eyeRx, eyeRy) - wall - pupilR;
    if (h < 0.5 || h < deadZone)
        return QPointF(0, 0);
    const qreal sina = nx / h;
    const qreal cosa = ny / h;
    qreal temp = qSqrt(qPow(eyeRx * sina, 2) + qPow(eyeRy * cosa, 2));
    temp -= pupilR;
    temp -= wall / 2.0;
    if (temp < 0)
        temp = 0;
    return QPointF(temp * sina, temp * cosa);
}

// ---------------------------------------------------------------------------
// 眼睛绘制（对齐 main.qml 236-460）
// ---------------------------------------------------------------------------
void drawEye(QPainter *p, const QRectF &rect, qreal dockSize,
             const EyeTheme &t, const EyeFrame &f)
{
    if (dockSize <= 0)
        return;

    QRectF body(0, 0, dockSize, dockSize * 0.8);
    body.moveCenter(rect.center());

    p->save();
    // blink / squint 纵比缩放（绕 eyeBody 中心）
    p->translate(body.center());
    p->scale(1.0, f.blink * f.squint);
    p->translate(-body.center());

    const qreal wall = qMax(2.0, dockSize * 0.07);

    if (!t.dbl) {
        // ---------- 单眼 ----------
        const qreal scleraRadius = body.height() / 2.0;
        fillRounded(p, body, scleraRadius, t.sclera, wall, t.border);

        p->save();
        p->setClipPath(roundedPath(body, scleraRadius));

        const qreal irisD = dockSize * 0.58;
        QRectF iris(0, 0, irisD, irisD);
        iris.moveCenter(body.center() + QPointF(f.dx, f.dy));
        p->setPen(Qt::NoPen);
        p->setBrush(t.iris);
        p->drawEllipse(iris);

        const qreal pw = t.slit ? irisD * 0.30 : irisD * 0.52;
        const qreal ph = t.slit ? irisD * 0.68 : pw;
        QRectF pupil(0, 0, pw, ph);
        pupil.moveCenter(iris.center());
        p->setBrush(t.pupil);
        p->drawPath(roundedPath(pupil, pw / 2.0));

        const qreal hlD = irisD * 0.22;
        QRectF hl(iris.center().x() + irisD * 0.26 - hlD / 2.0,
                  iris.top() + irisD * 0.16, hlD, hlD);
        p->setBrush(t.hl);
        p->drawEllipse(hl);

        if (t.lid) {
            const qreal lidH = body.height() * 0.30;
            QRectF lid(body.left(), body.top(), body.width(), lidH);
            fillRounded(p, lid, lidH * 0.9, t.lidColor);
            if (t.lashes) {
                const qreal lw = lid.width() * 0.10;
                fillRounded(p, QRectF(lid.left() + lid.width() * 0.20,
                                      lid.bottom() - lidH * 0.25, lw, lidH * 0.55),
                            lw / 2.0, t.border);
                fillRounded(p, QRectF(lid.center().x() - lw / 2.0,
                                      lid.bottom() - lidH * 0.30, lw, lidH * 0.70),
                            lw / 2.0, t.border);
                fillRounded(p, QRectF(lid.right() - lid.width() * 0.20 - lw,
                                      lid.bottom() - lidH * 0.25, lw, lidH * 0.55),
                            lw / 2.0, t.border);
            }
        }
        p->restore();
        p->restore();
        return;
    }

    // ---------- 双眼 ----------
    const qreal ew = body.width() * (t.round ? 0.47 : 0.46);
    const qreal eh = t.round ? body.height() * 0.95 : body.height();
    const qreal leftX = body.left() + body.width() * (t.round ? 0.015 : 0.02);
    const qreal rightX = body.left() + body.width() * (t.round ? 0.515 : (1.0 - 0.02 - 0.46));
    const qreal eyeY = body.center().y() - eh / 2.0;
    const qreal scleraRadius = t.round ? ew / 2.0 : eh * 0.42;
    const qreal borderW = qMax(2.0, dockSize * 0.06);

    const struct { qreal x; qreal dx; qreal dy; } eyes[2] = {
        { leftX,  f.dxL, f.dyL },
        { rightX, f.dxR, f.dyR },
    };

    for (const auto &e : eyes) {
        const QRectF eye(e.x, eyeY, ew, eh);
        fillRounded(p, eye, scleraRadius, t.sclera, borderW, t.border);

        p->save();
        p->setClipPath(roundedPath(eye, scleraRadius));

        const qreal irisD = ew * t.irisScale;
        QRectF iris(0, 0, irisD, irisD);
        iris.moveCenter(eye.center() + QPointF(e.dx, e.dy));
        p->setPen(Qt::NoPen);
        p->setBrush(t.iris);
        p->drawEllipse(iris);

        const qreal pupilD = irisD * t.pupilScale;
        QRectF pupil(0, 0, pupilD, pupilD);
        pupil.moveCenter(iris.center());
        p->setBrush(t.pupil);
        p->drawEllipse(pupil);

        const qreal hlD = irisD * 0.26;
        QRectF hl(iris.center().x() + irisD * 0.24 - hlD / 2.0,
                  iris.top() + irisD * 0.14, hlD, hlD);
        p->setBrush(t.hl);
        p->drawEllipse(hl);

        p->restore();
    }

    p->restore();
}

// ---------------------------------------------------------------------------
// 宠物绘制（对齐 main.qml 951-1269）
// ---------------------------------------------------------------------------
namespace {

// 单只宠物眼睛（component PetEye），自带独立相位的眨眼
void drawPetEye(QPainter *p, const QRectF &r, int phaseIndex, qint64 clockMs)
{
    const qreal d = qMin(r.width(), r.height());
    if (d <= 0)
        return;
    QRectF circ(0, 0, d, d);
    circ.moveCenter(r.center());
    const qreal s = d;
    const qreal cycle = 2200 + double(phaseIndex * 617 % 3200);
    const qreal ph = modPos(qreal(clockMs) + phaseIndex * 900.0, cycle) / cycle;
    qreal ys = 1.0;
    if (ph < 0.5) {
        ys = 1.0 - 0.9 * (ph / 0.5);
    } else {
        ys = 0.1 + 0.9 * ((ph - 0.5) / 0.5);
    }

    p->save();
    p->translate(circ.center());
    p->scale(1.0, ys);
    p->translate(-circ.center());

    fillRounded(p, circ, s / 2.0, QColor("#FFFFFF"), 0.6, QColor("#333333"));
    QRectF pupil(0, 0, s * 0.5, s * 0.5);
    pupil.moveCenter(circ.center());
    p->setPen(Qt::NoPen);
    p->setBrush(QColor("#111111"));
    p->drawEllipse(pupil);
    p->restore();
}

} // namespace

void drawPet(QPainter *p, const QRectF &rect, qreal dockSize, int petIndex, qint64 clockMs)
{
    if (petIndex < 0 || petIndex >= petDefs().size() || dockSize <= 0)
        return;

    const PetDef &pet = petDefs().at(petIndex);
    const qreal W = dockSize * 0.96;

    QRectF box(0, 0, W, W);
    box.moveCenter(rect.center());

    // 整体浮动 + 轻微摇摆（main.qml: 上下 ±3 / 旋转 ±3）
    const qreal floatCycle = 1600.0;
    const qreal tf = modPos(qreal(clockMs), floatCycle) / floatCycle;
    const qreal dy = -3.0 * (tf < 0.5 ? tf * 2.0 : (1.0 - tf) * 2.0);
    const qreal rot = 3.0 * qSin(2.0 * M_PI * qreal(clockMs) / 3300.0);

    p->save();
    p->translate(box.center() + QPointF(0, dy));
    p->rotate(rot);

    // 蛇：整体扭动
    if (pet.bodyAnim == 1) {
        const qreal bodyRot = pet.bodyA * qSin(2.0 * M_PI * qreal(clockMs) / (2.0 * pet.bodyMs));
        p->rotate(bodyRot);
    }

    int eyeIdx = 0;
    for (const PetOp &op : pet.ops) {
        qreal x = op.x * W;
        qreal y = op.y * W;
        qreal w = op.w * W;
        qreal h = op.absH ? op.h : op.h * W;
        qreal extraRot = op.rotation;

        if (op.anim == pet::kWag) {
            const qreal mid = (op.animA + op.animB) / 2.0;
            const qreal amp = (op.animA - op.animB) / 2.0;
            extraRot += mid + amp * qSin(2.0 * M_PI * qreal(clockMs) / (2.0 * op.animMs));
        } else if (op.anim == pet::kSwayX) {
            x += op.animA * qSin(2.0 * M_PI * qreal(clockMs) / (2.0 * op.animMs));
        }

        // 坐标以「宠物容器中心」为原点（QML 里子项的 x/y 以父项左上角为原点）
        const qreal ox = -W / 2.0;
        const qreal oy = -W / 2.0;

        if (op.kind == PetOp::Rect) {
            QRectF r(ox + x, oy + y, w, h);
            const qreal rad = op.radiusAbs ? op.radius : op.radius * W;
            p->save();
            if (qAbs(extraRot) > 0.001) {
                p->translate(r.center());
                p->rotate(extraRot);
                p->translate(-r.center());
            }
            fillRounded(p, r, rad, op.color, op.borderW, op.border);
            p->restore();
        } else if (op.kind == PetOp::Eye) {
            QRectF r(ox + x, oy + y, op.s * W, op.s * W);
            drawPetEye(p, r, eyeIdx++, clockMs);
        } else if (op.kind == PetOp::Text) {
            QFont f = p->font();
            f.setPixelSize(qMax(6, int(op.fontRatio * W)));
            f.setBold(true);
            p->setFont(f);
            p->setPen(op.color);
            p->drawText(QPointF(ox + x, oy + y + op.fontRatio * W), op.text);
        }
    }

    p->restore();
}

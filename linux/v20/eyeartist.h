// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// deepin V20 (dde-dock) 版卡通眼珠插件 —— 绘制层接口。
//
// 本文件把 V25 (dde-shell / QML) 版 main.qml 中的
//   · 20 套眼睛主题表
//   · 14 只宠物（十二生肖 + 鱼 + 猫）的矩形/眼睛/文字绘制项
//   · GEyes 瞳孔跟随算法
// 逐项搬到 C++，用 QPainter 绘制，避免依赖 QML 运行时。

#pragma once

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class QPainter;

// ---------------------------------------------------------------------------
// 眼睛主题（对应 main.qml 的 themes 表，共 20 条）
// ---------------------------------------------------------------------------
struct EyeTheme
{
    QString name;
    bool dbl = false;        // 双眼睛
    QColor sclera;           // 眼白
    QColor border;           // 眼眶
    QColor iris;             // 虹膜
    QColor pupil;            // 瞳孔
    QColor hl;               // 高光
    bool lid = false;        // 上眼睑
    QColor lidColor;
    bool lashes = false;     // 睫毛
    bool slit = false;       // 猫瞳竖缝
    bool round = false;      // 双眼主题圆眼
    qreal irisScale = 0.60;  // 双眼虹膜比例
    qreal pupilScale = 0.52; // 双眼瞳孔比例
};

// 主题表（顺序即菜单顺序，索引即 eyeStyle）
const QVector<EyeTheme> &eyeThemes();

// ---------------------------------------------------------------------------
// 宠物绘制项（对应 main.qml 里每只宠物的一串 Rectangle / PetEye / Text）
// ---------------------------------------------------------------------------
struct PetOp
{
    enum Kind { Rect, Eye, Text };

    Kind kind = Rect;

    // 位置与尺寸：x/y/w/h 均为「相对宠物容器宽度」的比例；absH=true 时 h 为绝对像素
    qreal x = 0;
    qreal y = 0;
    qreal w = 0;
    qreal h = 0;
    qreal radius = 0;            // radiusAbs=false 时为「相对宽度」的比例
    bool radiusAbs = false;      // radius 为绝对像素（QML 里的 radius: 1 / 2 / 3）
    qreal rotation = 0;          // 度，绕自身中心
    bool absH = false;

    QColor color;
    QColor border;
    qreal borderW = 0;

    // Eye：眼睛直径 = 容器宽度 * s
    qreal s = 0;

    // Text
    QString text;
    qreal fontRatio = 0;

    // 动画：0=无 1=绕中心摆动旋转（animA↔animB） 2=水平游动（±animA）
    int anim = 0;
    qreal animA = 0;
    qreal animB = 0;
    int animMs = 800;

    static PetOp rect(qreal x, qreal y, qreal w, qreal h, qreal radius,
                      const QString &color, qreal borderW = 0,
                      const QString &borderColor = QString(), qreal rotation = 0,
                      bool absH = false, bool radiusAbs = false,
                      int anim = 0, qreal animA = 0, qreal animB = 0, int animMs = 800);
    static PetOp eye(qreal x, qreal y, qreal s);
    static PetOp label(qreal x, qreal y, qreal fontRatio, const QString &t,
                       const QString &color);
};

struct PetDef
{
    QString name;          // 菜单名（含 emoji）
    QVector<PetOp> ops;    // 按 QML 文档顺序，后面的盖在前面上
    // 整体动画（蛇身扭动）：0=无 1=绕中心旋转 ±animA
    int bodyAnim = 0;
    qreal bodyA = 6;
    int bodyMs = 700;
};

// 宠物表（索引即 petStyle，0..13）
const QVector<PetDef> &petDefs();

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------

// 眼睛瞳距偏移（GEyes 算法，与 main.qml computePupilOffset 逐行对齐）
QPointF computePupilOffset(qreal localX, qreal localY, qreal cx, qreal cy,
                           qreal eyeRx, qreal eyeRy, qreal pupilR, qreal wall);

// 单帧状态：由 EyeWidget 计算后交给绘制函数
struct EyeFrame
{
    qreal dx = 0, dy = 0;      // 单眼虹膜偏移
    qreal dxL = 0, dyL = 0;    // 左眼
    qreal dxR = 0, dyR = 0;    // 右眼
    qreal blink = 1.0;         // 眨眼纵比
    qreal squint = 1.0;        // 到边缘眯眼纵比
};

// 在 rect 内绘制眼睛（rect 为整块任务栏插件面积，dockSize 为其中皮肤尺寸）
void drawEye(QPainter *p, const QRectF &rect, qreal dockSize,
             const EyeTheme &theme, const EyeFrame &f);

// 在 rect 内绘制宠物；animMs 为当前动画时钟（毫秒，用于摆动/游动相位）
void drawPet(QPainter *p, const QRectF &rect, qreal dockSize, int petIndex,
             qint64 clockMs);

// 眼睛样式数量 / 宠物数量
int eyeThemeCount();
int petCount();

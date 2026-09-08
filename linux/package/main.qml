// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 瞳孔跟随参照 gnome-applets GEyes 算法（HiDPI 适配 / 椭圆约束 / 死区）
// 多主题参照 GEyes themes/ + 哆啦A梦 / 圆眼双瞳
// 学习功能：左键弹出随机学习内容（诗经/唐诗/宋词/雅思/托福/四六级/高考/2000词），
// 右键菜单分两级（眼睛样式 / 学习内容）；英文单词与例句旁小喇叭可网络TTS朗读。

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.ds 1.0
import org.deepin.ds.dock 1.0
import org.deepin.dtk 1.0 as D

AppletItem {
    id: root

    // ============ Dock 布局：显示位置（0左 1中 2右） ============
    property int posPref: Applet.posPref
    property int dockOrder: posPref === 0 ? 5 : (posPref === 1 ? 13 : 28)
    property int dockPosition: posPref === 2 ? 1 : 0
    property bool shouldVisible: Applet.visible && Applet.supported
    readonly property bool isVerticalDock: Panel.position === Dock.Left || Panel.position === Dock.Right
    readonly property real dockSize: Panel.rootObject ? Panel.rootObject.dockItemMaxSize : 40

    implicitWidth: dockSize
    implicitHeight: dockSize

    // ============ 主题系统（参照 GEyes themes/） ============
    property var themes: [
        { name: "蓝色卡通",    double: false, sclera: "#FDFDFD", border: "#262626", iris: "#3E6FB0", pupil: "#151515", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "双眼睛",      double: true,  sclera: "#FFFFFF", border: "#2B2B2B", iris: "#33231A", pupil: "#151515", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "绿眸少女",    double: false, sclera: "#F0F8F0", border: "#2E7D32", iris: "#43A047", pupil: "#0B3D0B", hl: "#FFFFFF", lid: true,  lidColor: "#FFE0B2", lashes: false, slit: false },
        { name: "粉眸少女",    double: false, sclera: "#FFF0F5", border: "#D81B60", iris: "#F06292", pupil: "#7B1E3C", hl: "#FFFFFF", lid: true,  lidColor: "#FFD9E8", lashes: false, slit: false },
        { name: "浓睫大眼",    double: false, sclera: "#FFFFFF", border: "#1A1A1A", iris: "#1976D2", pupil: "#0D1B2A", hl: "#FFFFFF", lid: true,  lidColor: "#FFE0B2", lashes: true,  slit: false },
        { name: "恶魔红眼",    double: false, sclera: "#FDE8E8", border: "#8E0000", iris: "#C62828", pupil: "#3E0000", hl: "#FFD0D0", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "恐怖之眼",    double: false, sclera: "#141414", border: "#000000", iris: "#FFD600", pupil: "#1A1A00", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "南瓜怪眼",    double: false, sclera: "#FFF3E0", border: "#B4540A", iris: "#E67E22", pupil: "#4A2400", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "奇异双瞳",    double: true,  sclera: "#EEEEEE", border: "#424242", iris: "#7E57C2", pupil: "#1A1033", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "猫瞳",        double: false, sclera: "#F1F8E9", border: "#33691E", iris: "#9CCC65", pupil: "#1B3A0B", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: true  },
        { name: "哆啦A梦",     double: true,  sclera: "#FFFFFF", border: "#222222", iris: "#1E1E1E", pupil: "#000000", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false, round: true, irisScale: 0.55, pupilScale: 0.80 },
        { name: "圆眼双瞳",    double: true,  sclera: "#FFFFFF", border: "#2B2B2B", iris: "#1976D2", pupil: "#0D1B2A", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false, round: true, irisScale: 0.55, pupilScale: 0.50 },
        { name: "蓝色卡通·双眼", double: true, sclera: "#FDFDFD", border: "#262626", iris: "#3E6FB0", pupil: "#151515", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "绿眸少女·双眼", double: true, sclera: "#F0F8F0", border: "#2E7D32", iris: "#43A047", pupil: "#0B3D0B", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "粉眸少女·双眼", double: true, sclera: "#FFF0F5", border: "#D81B60", iris: "#F06292", pupil: "#7B1E3C", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "浓睫大眼·双眼", double: true, sclera: "#FFFFFF", border: "#1A1A1A", iris: "#1976D2", pupil: "#0D1B2A", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "恶魔红眼·双眼", double: true, sclera: "#FDE8E8", border: "#8E0000", iris: "#C62828", pupil: "#3E0000", hl: "#FFD0D0", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "恐怖之眼·双眼", double: true, sclera: "#141414", border: "#000000", iris: "#FFD600", pupil: "#1A1A00", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "南瓜怪眼·双眼", double: true, sclera: "#FFF3E0", border: "#B4540A", iris: "#E67E22", pupil: "#4A2400", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false },
        { name: "猫瞳·双眼",    double: true, sclera: "#F1F8E9", border: "#33691E", iris: "#9CCC65", pupil: "#1B3A0B", hl: "#FFFFFF", lid: false, lidColor: "#F2C9A0", lashes: false, slit: false }
    ]
    property int eyeStyle: Applet.eyeStyle
    property var theme: themes[eyeStyle]

    // ============ GEyes 瞳孔偏移计算 ============
    function computePupilOffset(localX, localY, cx, cy, eyeRx, eyeRy, pupilR, wall) {
        var nx = localX - cx
        var ny = localY - cy
        var h = Math.sqrt(nx * nx + ny * ny)
        var deadZone = Math.min(eyeRx, eyeRy) - wall - pupilR
        if (h < 0.5 || h < deadZone) {
            return Qt.point(0, 0)
        }
        var sina = nx / h
        var cosa = ny / h
        var temp = Math.sqrt(Math.pow(eyeRx * sina, 2) + Math.pow(eyeRy * cosa, 2))
        temp -= pupilR
        temp -= wall / 2
        if (temp < 0) temp = 0
        return Qt.point(temp * sina, temp * cosa)
    }

    property real targetDx: 0
    property real targetDy: 0
    property real targetDxL: 0
    property real targetDyL: 0
    property real targetDxR: 0
    property real targetDyR: 0

    // ============ 静止检测 + 空闲游走 ============
    property real lastCursorX: -9999
    property real lastCursorY: -9999
    property real lastMoveTime: 0
    property bool idleWandering: false
    property real wanderX: 0
    property real wanderY: 0

    function pickWanderTarget() {
        wanderX = root.width / 2 + (Math.random() - 0.5) * root.width * 1.8
        wanderY = root.height / 2 + (Math.random() - 0.5) * root.height * 1.5
    }

    Timer {
        id: wanderTimer
        interval: 1800
        running: false
        repeat: true
        onTriggered: root.pickWanderTarget()
    }

    Component.onCompleted: lastMoveTime = Date.now()

    Timer {
        interval: 33
        running: true
        repeat: true
        onTriggered: {
            var dpr = Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
            var gx = Applet.cursorX / dpr
            var gy = Applet.cursorY / dpr

            if (Math.abs(gx - lastCursorX) > 0.5 || Math.abs(gy - lastCursorY) > 0.5) {
                lastCursorX = gx
                lastCursorY = gy
                lastMoveTime = Date.now()
                if (idleWandering) {
                    idleWandering = false
                    wanderTimer.stop()
                }
            } else if (!idleWandering && Date.now() - lastMoveTime > 4000) {
                idleWandering = true
                root.pickWanderTarget()
                wanderTimer.start()
            }

            var local = root.mapFromGlobal(gx, gy)
            var lx = idleWandering ? wanderX : local.x
            var ly = idleWandering ? wanderY : local.y
            var wall = Math.max(2, dockSize * 0.07)

            var off0 = root.computePupilOffset(lx, ly,
                        root.width / 2, root.height / 2,
                        root.width / 2, root.height / 2,
                        dockSize * 0.29, wall)
            targetDx = off0.x
            targetDy = off0.y

            var eyeW = dockSize * (root.theme.round ? 0.47 : 0.46)
            var eyeH = root.theme.round ? dockSize * 0.95 : root.height
            var cy = root.height / 2
            var lcx = dockSize * 0.25
            var rcx = dockSize * 0.75
            var irisR = eyeW * (root.theme.irisScale || 0.60) / 2 * 1.15
            var offL = root.computePupilOffset(lx, ly, lcx, cy,
                        eyeW / 2, eyeH / 2, irisR, wall)
            var offR = root.computePupilOffset(lx, ly, rcx, cy,
                        eyeW / 2, eyeH / 2, irisR, wall)
            targetDxL = offL.x
            targetDyL = offL.y
            targetDxR = offR.x
            targetDyR = offR.y

            var ratio0 = Math.sqrt(off0.x * off0.x + off0.y * off0.y)
                        / Math.max(1, root.width / 2 - dockSize * 0.29 - wall)
            var ratio1 = Math.max(Math.sqrt(offL.x * offL.x + offL.y * offL.y),
                                  Math.sqrt(offR.x * offR.x + offR.y * offR.y))
                        / Math.max(1, eyeW / 2 - irisR - wall)
            var ratio = root.theme.double ? ratio1 : ratio0
            squintScale.yScale = ratio > 0.8 ? 0.88 : 1.0
        }
    }

    // ============ 随机眨眼 ============
    Timer {
        id: blinkTimer
        interval: 3000
        running: true
        repeat: true
        onTriggered: {
            var r = Math.random()
            if (r < 0.18) blinkFast.restart()
            else if (r < 0.38) blinkDouble.restart()
            else blinkNormal.restart()
            interval = 2200 + Math.random() * 4800
        }
    }
    SequentialAnimation {
        id: blinkNormal
        running: false
        NumberAnimation { target: eyeScale; property: "yScale"; to: 0.06; duration: 85; easing.type: Easing.InOutQuad }
        NumberAnimation { target: eyeScale; property: "yScale"; to: 1.0;  duration: 120; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
        id: blinkFast
        running: false
        NumberAnimation { target: eyeScale; property: "yScale"; to: 0.06; duration: 45; easing.type: Easing.InOutQuad }
        NumberAnimation { target: eyeScale; property: "yScale"; to: 1.0;  duration: 60; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
        id: blinkDouble
        running: false
        NumberAnimation { target: eyeScale; property: "yScale"; to: 0.06; duration: 70; easing.type: Easing.InOutQuad }
        NumberAnimation { target: eyeScale; property: "yScale"; to: 1.0;  duration: 45; easing.type: Easing.OutCubic }
        PauseAnimation { duration: 40 }
        NumberAnimation { target: eyeScale; property: "yScale"; to: 0.06; duration: 70; easing.type: Easing.InOutQuad }
        NumberAnimation { target: eyeScale; property: "yScale"; to: 1.0;  duration: 120; easing.type: Easing.OutCubic }
    }

    // ============ 眼睛绘制 ============
    Item {
        id: eyeBody
        visible: root.petStyle < 0
        width: dockSize
        height: dockSize * 0.8
        anchors.centerIn: parent

        transform: [
            Scale {
                id: eyeScale
                origin.x: eyeBody.width / 2
                origin.y: eyeBody.height / 2
                xScale: 1.0
                yScale: 1.0
            },
            Scale {
                id: squintScale
                origin.x: eyeBody.width / 2
                origin.y: eyeBody.height / 2
                xScale: 1.0
                yScale: 1.0
                Behavior on yScale {
                    NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                }
            }
        ]

        // ---------- 单眼主题 ----------
        Item {
            visible: !root.theme.double
            anchors.fill: parent

            Rectangle {
                id: sclera
                anchors.fill: parent
                radius: height / 2
                color: root.theme.sclera
                border.width: Math.max(2, dockSize * 0.07)
                border.color: root.theme.border
            }

            Item {
                anchors.fill: parent
                clip: true

                Rectangle {
                    id: iris
                    width: dockSize * 0.58
                    height: width
                    radius: width / 2
                    color: root.theme.iris
                    x: parent.width / 2 - width / 2 + root.targetDx
                    y: parent.height / 2 - height / 2 + root.targetDy

                    Behavior on x { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }
                    Behavior on y { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }

                    Rectangle {
                        width: root.theme.slit ? parent.width * 0.30 : parent.width * 0.52
                        height: root.theme.slit ? parent.width * 0.68 : width
                        radius: width / 2
                        color: root.theme.pupil
                        anchors.centerIn: parent
                    }
                    Rectangle {
                        width: parent.width * 0.22
                        height: width
                        radius: width / 2
                        color: root.theme.hl
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: parent.width * 0.26
                        anchors.top: parent.top
                        anchors.topMargin: parent.height * 0.16
                    }
                }

                Rectangle {
                    visible: root.theme.lid
                    width: parent.width
                    height: parent.height * 0.30
                    y: 0
                    radius: height * 0.9
                    color: root.theme.lidColor

                    Rectangle {
                        visible: root.theme.lashes
                        width: parent.width * 0.10
                        height: parent.height * 0.55
                        radius: width / 2
                        color: root.theme.border
                        anchors.top: parent.bottom
                        anchors.topMargin: -parent.height * 0.25
                        anchors.left: parent.left
                        anchors.leftMargin: parent.width * 0.20
                    }
                    Rectangle {
                        visible: root.theme.lashes
                        width: parent.width * 0.10
                        height: parent.height * 0.70
                        radius: width / 2
                        color: root.theme.border
                        anchors.top: parent.bottom
                        anchors.topMargin: -parent.height * 0.30
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        visible: root.theme.lashes
                        width: parent.width * 0.10
                        height: parent.height * 0.55
                        radius: width / 2
                        color: root.theme.border
                        anchors.top: parent.bottom
                        anchors.topMargin: -parent.height * 0.25
                        anchors.right: parent.right
                        anchors.rightMargin: parent.width * 0.20
                    }
                }
            }
        }

        // ---------- 双眼主题（支持 round 圆眼 / irisScale / pupilScale） ----------
        Item {
            visible: root.theme.double
            anchors.fill: parent

            // 左眼
            Item {
                width: root.theme.round ? parent.width * 0.47 : parent.width * 0.46
                height: root.theme.round ? parent.height * 0.95 : parent.height
                anchors.verticalCenter: parent.verticalCenter
                x: root.theme.round ? parent.width * 0.015 : parent.width * 0.02

                Rectangle {
                    anchors.fill: parent
                    radius: root.theme.round ? width / 2 : height * 0.42
                    color: root.theme.sclera
                    border.width: Math.max(2, dockSize * 0.06)
                    border.color: root.theme.border
                }
                Item {
                    anchors.fill: parent
                    clip: true
                    Rectangle {
                        width: parent.width * (root.theme.irisScale || 0.60)
                        height: width
                        radius: width / 2
                        color: root.theme.iris
                        x: parent.width / 2 - width / 2 + root.targetDxL
                        y: parent.height / 2 - height / 2 + root.targetDyL

                        Behavior on x { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }
                        Behavior on y { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }

                        Rectangle {
                            width: parent.width * (root.theme.pupilScale || 0.52)
                            height: width
                            radius: width / 2
                            color: root.theme.pupil
                            anchors.centerIn: parent
                        }
                        Rectangle {
                            width: parent.width * 0.26
                            height: width
                            radius: width / 2
                            color: root.theme.hl
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.horizontalCenterOffset: parent.width * 0.24
                            anchors.top: parent.top
                            anchors.topMargin: parent.height * 0.14
                        }
                    }
                }
            }

            // 右眼
            Item {
                width: root.theme.round ? parent.width * 0.47 : parent.width * 0.46
                height: root.theme.round ? parent.height * 0.95 : parent.height
                anchors.verticalCenter: parent.verticalCenter
                x: root.theme.round ? parent.width * 0.515 : parent.width * (1 - 0.02 - 0.46)

                Rectangle {
                    anchors.fill: parent
                    radius: root.theme.round ? width / 2 : height * 0.42
                    color: root.theme.sclera
                    border.width: Math.max(2, dockSize * 0.06)
                    border.color: root.theme.border
                }
                Item {
                    anchors.fill: parent
                    clip: true
                    Rectangle {
                        width: parent.width * (root.theme.irisScale || 0.60)
                        height: width
                        radius: width / 2
                        color: root.theme.iris
                        x: parent.width / 2 - width / 2 + root.targetDxR
                        y: parent.height / 2 - height / 2 + root.targetDyR

                        Behavior on x { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }
                        Behavior on y { SpringAnimation { spring: 2.5; damping: 0.42; mass: 1.4 } }

                        Rectangle {
                            width: parent.width * (root.theme.pupilScale || 0.52)
                            height: width
                            radius: width / 2
                            color: root.theme.pupil
                            anchors.centerIn: parent
                        }
                        Rectangle {
                            width: parent.width * 0.26
                            height: width
                            radius: width / 2
                            color: root.theme.hl
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.horizontalCenterOffset: parent.width * 0.24
                            anchors.top: parent.top
                            anchors.topMargin: parent.height * 0.14
                        }
                    }
                }
            }
        }
    }

    // ============ 学习内容弹窗 ============
    // 深/浅色跟随系统外观
    readonly property bool darkMode: D.DTK.themeType === D.ApplicationHelper.DarkType
    readonly property color popupBg: darkMode ? "#232529" : "#F7F9FC"
    readonly property color popupBorder: darkMode ? "#3A3D42" : Qt.rgba(0, 0, 0, 0.12)
    readonly property color textMain: darkMode ? "#E8EAED" : "#1F2329"
    readonly property color textBody: darkMode ? "#C9CDD4" : "#333333"
    readonly property color textMuted: darkMode ? "#8E939B" : "#777777"
    readonly property color textSub: darkMode ? "#9AA0A8" : "#8A919F"
    readonly property color accentColor: darkMode ? "#5B8FD9" : "#3E6FB0"
    readonly property color btnBg: darkMode ? "#34373D" : "#E8EDF3"
    readonly property color dividerColor: darkMode ? "#34373D" : "#E4E7ED"

    property var learnItem: ({})
    property bool learnEmpty: false

    // 只更新数据（不重新 open，避免"换一个"失效）
    function nextLearningItem() {
        var item = Applet.nextItem()
        if (item && Object.keys(item).length > 0) {
            learnEmpty = false
            learnItem = item
        } else {
            learnEmpty = true
        }
        if (learnFlick) {
            learnFlick.contentY = 0
        }
        Qt.callLater(root.updateLearnPopupHeight)
    }

    // 按任务栏位置调整弹窗位置（顶部→下方，底部→上方，左/右侧→旁边）
    function positionPopup(pop) {
        var pt = root.mapToItem(null, 0, 0)
        if (Panel.position === Dock.Top) {
            pop.popupX = pt.x
            pop.popupY = pt.y + root.height + 10
        } else if (Panel.position === Dock.Left) {
            pop.popupX = pt.x + root.width + 10
            pop.popupY = pt.y
        } else if (Panel.position === Dock.Right) {
            pop.popupX = pt.x - pop.width - 10
            pop.popupY = pt.y
        } else {
            pop.popupX = pt.x
            pop.popupY = pt.y - pop.height - 10
        }
    }

    function updatePopupPosition() {
        positionPopup(learnPopup)
    }

    // 弹窗高度由绑定自动调整；此函数在内容变化后触发重新定位
    function updateLearnPopupHeight() {
        if (learnPopup.popupVisible) {
            root.updatePopupPosition()
        }
    }

    // 内容隐式高度变化（换题/换行）时自动调整弹窗高度
    Connections {
        target: contentCol
        function onImplicitHeightChanged() {
            root.updateLearnPopupHeight()
        }
    }

    function showLearning() {
        nextLearningItem()
        learnPopup.open()
    }

    // 组装当前学习内容的完整文本（供复制）
    function currentContentText() {
        if (learnEmpty) {
            return ""
        }
        if (learnItem.type === "poem") {
            var s1 = (learnItem.t || "") + "\n" + (learnItem.a || "")
            if (learnItem.sec) {
                s1 += " · " + learnItem.sec
            }
            s1 += "\n\n" + (learnItem.txt || "")
            if (learnItem.trans) {
                s1 += "\n\n【译文】\n" + learnItem.trans
            }
            if (learnItem.note) {
                s1 += "\n\n【赏析·注释】\n" + learnItem.note
            }
            return s1
        }
        if (learnItem.type === "wiki") {
            return (learnItem.t || "") + "\n\n" + (learnItem.d || "") + "\n\n（来源：维基百科）"
        }
        var s2 = (learnItem.w || "")
        if (learnItem.p) {
            s2 += "  /" + learnItem.p + "/"
        }
        s2 += "\n" + (learnItem.d || "")
        var es = learnItem.es || []
        for (var i = 0; i < es.length; i++) {
            s2 += "\n\n例句" + (i + 1) + ": " + (es[i].e || "")
            if (es[i].c) {
                s2 += "\n" + es[i].c
            }
        }
        return s2
    }

    property bool copyFlash: false
    Timer {
        id: copyResetTimer
        interval: 1200
        repeat: false
        onTriggered: root.copyFlash = false
    }
    function copyCurrent() {
        var t = root.currentContentText()
        if (t === "") {
            return
        }
        Applet.copyText(t)
        copyFlash = true
        copyResetTimer.restart()
    }

    // 小喇叭按钮（点击朗读；空文本自动隐藏）
    component SpeakerButton: Rectangle {
        property string text: ""
        property string lang: "en"
        visible: text !== ""
        width: 26
        height: 26
        radius: 13
        color: root.btnBg
        border.width: 1
        border.color: root.popupBorder
        Text {
            anchors.centerIn: parent
            text: "🔊"
            font.pixelSize: 13
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
            onExited: parent.color = root.btnBg
            onClicked: {
                var t = text
                if (t !== "") {
                    Applet.speak(t, lang)
                }
            }
        }
    }

    PanelPopup {
        id: learnPopup
        width: 460
        // 高度自动跟随内容（绑定隐式高度，避免异步测量不准）
        height: Math.min(470, Math.max(260, 96 + (contentCol ? contentCol.implicitHeight : 160)))
        onHeightChanged: {
            if (popupVisible) {
                root.updatePopupPosition()
            }
        }
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y
        onPopupVisibleChanged: {
            if (popupVisible) {
                root.updatePopupPosition()
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: 14
            color: root.popupBg
            border.width: 1
            border.color: root.popupBorder

            // 顶栏
            RowLayout {
                id: learnHeader
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 14
                height: 28
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: Math.max(84, catLabel.implicitWidth + 18)
                    Layout.preferredHeight: 24
                    radius: 12
                    color: root.accentColor
                    Text {
                        id: catLabel
                        anchors.centerIn: parent
                        text: learnEmpty ? "学习内容" : (learnItem.catName || "")
                        color: "#FFFFFF"
                        font.pixelSize: 12
                    }
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: copyLabel.implicitWidth + 20
                    Layout.preferredHeight: 26
                    radius: 13
                    color: root.btnBg
                    border.width: 1
                    border.color: root.popupBorder
                    Text {
                        id: copyLabel
                        anchors.centerIn: parent
                        text: root.copyFlash ? "✓ 已复制" : "📋 复制"
                        font.pixelSize: 12
                        color: root.copyFlash ? "#2EA043" : root.accentColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: root.copyCurrent()
                    }
                }
                Rectangle {
                    Layout.preferredWidth: nextLabel.implicitWidth + 20
                    Layout.preferredHeight: 26
                    radius: 13
                    color: root.btnBg
                    border.width: 1
                    border.color: root.popupBorder
                    Text {
                        id: nextLabel
                        anchors.centerIn: parent
                        text: "🎲 换一个"
                        font.pixelSize: 12
                        color: root.accentColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: root.nextLearningItem()
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: 13
                    color: root.btnBg
                    border.width: 1
                    border.color: root.popupBorder
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: root.textSub
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: learnPopup.close()
                    }
                }
            }

            // 内容区（可滚动）
            Flickable {
                id: learnFlick
                anchors.top: learnHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 14
                clip: true
                contentHeight: contentCol.implicitHeight

                Column {
                    id: contentCol
                    width: parent.width
                    spacing: 8

                    // ---------- 诗歌内容（唐诗/宋词/诗经） ----------
                    Column {
                        visible: !learnEmpty && learnItem.type === "poem"
                        width: parent.width
                        spacing: 6

                        Text {
                            width: parent.width
                            text: learnItem.t || ""
                            font.pixelSize: 19
                            font.bold: true
                            color: root.textMain
                            wrapMode: Text.Wrap
                        }
                        Text {
                            width: parent.width
                            text: {
                                var a = learnItem.a || ""
                                var s = learnItem.sec || ""
                                return a + (s !== "" ? " · " + s : "")
                            }
                            font.pixelSize: 12
                            color: root.textSub
                            wrapMode: Text.Wrap
                        }
                        Text {
                            width: parent.width
                            text: learnItem.txt || ""
                            font.pixelSize: 15
                            color: root.textBody
                            lineHeight: 1.5
                            wrapMode: Text.Wrap
                        }
                        Rectangle { width: parent.width; height: 1; color: root.dividerColor }

                        // 译文（诗经无译文时隐藏）
                        Column {
                            visible: (learnItem.trans || "") !== ""
                            width: parent.width
                            spacing: 4
                            Text { text: "【译文】"; font.pixelSize: 12; font.bold: true; color: root.accentColor }
                            Text {
                                width: parent.width
                                text: learnItem.trans || ""
                                font.pixelSize: 13
                                color: root.textBody
                                lineHeight: 1.4
                                wrapMode: Text.Wrap
                            }
                        }

                        // 赏析/注释（无时隐藏）
                        Column {
                            visible: (learnItem.note || "") !== ""
                            width: parent.width
                            spacing: 4
                            Text { text: "【赏析·注释】"; font.pixelSize: 12; font.bold: true; color: root.accentColor }
                            Text {
                                width: parent.width
                                text: learnItem.note || ""
                                font.pixelSize: 13
                                color: root.textMuted
                                lineHeight: 1.4
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    // ---------- 单词内容 ----------
                    Column {
                        visible: !learnEmpty && learnItem.type === "word"
                        width: parent.width
                        spacing: 6
                        Row {
                            width: parent.width
                            spacing: 10
                            Text {
                                text: learnItem.w || ""
                                font.pixelSize: 24
                                font.bold: true
                                color: root.textMain
                            }
                            SpeakerButton {
                                text: learnItem.w || ""
                                lang: "en"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Text {
                            width: parent.width
                            text: "/" + (learnItem.p || "") + "/"
                            font.pixelSize: 13
                            color: root.textSub
                            wrapMode: Text.Wrap
                        }
                        Text {
                            width: parent.width
                            text: learnItem.d || ""
                            font.pixelSize: 14
                            color: root.textBody
                            lineHeight: 1.4
                            wrapMode: Text.Wrap
                        }
                        Rectangle { width: parent.width; height: 1; color: root.dividerColor }
                        // 例句列表（2-3 条）
                        Repeater {
                            model: learnItem.es || []
                            delegate: Column {
                                width: parent.width
                                spacing: 3
                                Row {
                                    width: parent.width
                                    spacing: 8
                                    Text {
                                        text: "例句" + (index + 1)
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: root.accentColor
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    SpeakerButton {
                                        text: modelData.e || ""
                                        lang: "en"
                                        width: 22
                                        height: 22
                                        radius: 11
                                    }
                                }
                                Text {
                                    width: parent.width
                                    text: modelData.e || ""
                                    font.pixelSize: 14
                                    font.italic: true
                                    color: root.textBody
                                    lineHeight: 1.4
                                    wrapMode: Text.Wrap
                                }
                                Text {
                                    width: parent.width
                                    text: modelData.c || ""
                                    font.pixelSize: 13
                                    color: root.textMuted
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }


                    // ---------- 维基百科词条内容 ----------
                    Column {
                        visible: !learnEmpty && learnItem.type === "wiki"
                        width: parent.width
                        spacing: 8
                        Text {
                            width: parent.width
                            text: "📖 " + (learnItem.t || "")
                            font.pixelSize: 20
                            font.bold: true
                            color: root.textMain
                            wrapMode: Text.Wrap
                        }
                        Rectangle { width: parent.width; height: 1; color: root.dividerColor }
                        Text {
                            width: parent.width
                            text: learnItem.d || ""
                            font.pixelSize: 14
                            color: root.textBody
                            lineHeight: 1.6
                            wrapMode: Text.Wrap
                        }
                        Text {
                            width: parent.width
                            text: "（来源：维基百科）"
                            font.pixelSize: 11
                            color: root.textMuted
                            wrapMode: Text.Wrap
                        }
                    }

                    // ---------- 未选择分类提示 ----------
                    Text {
                        visible: learnEmpty
                        width: parent.width
                        text: "还没有选择学习内容 🤔\n\n请 右键点击眼睛 → 学习内容\n勾选要学习的分类后再点眼睛"
                        font.pixelSize: 14
                        color: root.textSub
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        lineHeight: 1.6
                    }
                }
            }
        }
    }

    // ============ 宠物模式（十二生肖 + 鱼 + 猫） ============
    property int petStyle: Applet.petStyle

    // 宠物眼睛（白色+黑瞳，随机眨眼）
    component PetEye: Item {
        property real s: 4
        width: s
        height: s
        transform: Scale {
            id: petEyeSc
            origin.x: s / 2
            origin.y: s / 2
            xScale: 1
            yScale: 1
        }
        Rectangle {
            anchors.fill: parent
            radius: s / 2
            color: "#FFFFFF"
            border.width: 0.6
            border.color: "#333333"
        }
        Rectangle {
            width: s * 0.5
            height: s * 0.5
            radius: s * 0.25
            color: "#111111"
            anchors.centerIn: parent
        }
        Timer {
            interval: 2200 + Math.random() * 3200
            running: true
            repeat: true
            onTriggered: {
                petEyeBlink.restart()
                interval = 2200 + Math.random() * 3200
            }
        }
        SequentialAnimation {
            id: petEyeBlink
            running: false
            NumberAnimation { target: petEyeSc; property: "yScale"; to: 0.1; duration: 70; easing.type: Easing.InOutQuad }
            NumberAnimation { target: petEyeSc; property: "yScale"; to: 1.0; duration: 90; easing.type: Easing.OutQuad }
        }
    }

    // 宠物主体容器（浮动 + 摇摆动画）
    Item {
        visible: root.petStyle >= 0
        anchors.fill: parent

        Item {
            id: petFloat
            anchors.centerIn: parent
            width: dockSize * 0.96
            height: dockSize * 0.96

            SequentialAnimation on y {
                running: root.petStyle >= 0
                loops: Animation.Infinite
                NumberAnimation { to: -3; duration: 800; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 0;  duration: 800; easing.type: Easing.InOutQuad }
            }
            SequentialAnimation on rotation {
                running: root.petStyle >= 0
                loops: Animation.Infinite
                NumberAnimation { to: -3; duration: 1300; easing.type: Easing.InOutSine }
                NumberAnimation { to: 3;  duration: 1300; easing.type: Easing.InOutSine }
                NumberAnimation { to: 0;  duration: 700; easing.type: Easing.InOutSine }
            }

            // ---- 鼠 ----
            Item {
                visible: root.petStyle === 0
                anchors.fill: parent
                Rectangle { width: parent.width * 0.30; height: parent.width * 0.30; radius: parent.width*0.15; color: "#C9A9A6"; border.width: 1; border.color: "#7A5C58"; x: parent.width*0.06; y: parent.width*0.02 }
                Rectangle { width: parent.width * 0.30; height: parent.width * 0.30; radius: parent.width*0.15; color: "#C9A9A6"; border.width: 1; border.color: "#7A5C58"; x: parent.width*0.64; y: parent.width*0.02 }
                Rectangle { width: parent.width * 0.72; height: parent.width * 0.72; radius: parent.width*0.36; color: "#D8BCB9"; border.width: 1.5; border.color: "#5C4644"; x: parent.width*0.14; y: parent.width*0.14 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.34; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.55; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.16; height: parent.width*0.10; radius: parent.width*0.05; color: "#F49CA0"; x: parent.width*0.42; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.05; height: parent.width*0.10; radius: parent.width*0.03; color: "#FFFFFF"; x: parent.width*0.40; y: parent.width*0.62 }
                Rectangle { width: parent.width*0.05; height: parent.width*0.10; radius: parent.width*0.03; color: "#FFFFFF"; x: parent.width*0.55; y: parent.width*0.62 }
                Rectangle { width: parent.width*0.16; height: 1; color: "#8A6A55"; x: parent.width*0.08; y: parent.width*0.50; rotation: 10 }
                Rectangle { width: parent.width*0.16; height: 1; color: "#8A6A55"; x: parent.width*0.76; y: parent.width*0.50; rotation: -10 }
            }
            // ---- 牛 ----
            Item {
                visible: root.petStyle === 1
                anchors.fill: parent
                Rectangle { width: parent.width*0.22; height: parent.width*0.14; radius: parent.width*0.07; color: "#8A6B3F"; x: parent.width*0.18; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.22; height: parent.width*0.14; radius: parent.width*0.07; color: "#8A6B3F"; x: parent.width*0.60; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.70; radius: parent.width*0.34; color: "#A97F4F"; border.width: 1.5; border.color: "#4E3A22"; x: parent.width*0.14; y: parent.width*0.16 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.35; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.55; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.18; height: parent.width*0.12; radius: parent.width*0.04; color: "#6B4A26"; x: parent.width*0.41; y: parent.width*0.58 }
                Rectangle { width: parent.width*0.03; height: parent.width*0.06; radius: 1; color: "#3A2712"; x: parent.width*0.43; y: parent.width*0.58 }
                Rectangle { width: parent.width*0.03; height: parent.width*0.06; radius: 1; color: "#3A2712"; x: parent.width*0.54; y: parent.width*0.58 }
            }
            // ---- 虎 ----
            Item {
                visible: root.petStyle === 2
                anchors.fill: parent
                Rectangle { width: parent.width*0.20; height: parent.width*0.14; radius: parent.width*0.07; color: "#D98E32"; x: parent.width*0.18; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.20; height: parent.width*0.14; radius: parent.width*0.07; color: "#D98E32"; x: parent.width*0.62; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.72; radius: parent.width*0.36; color: "#F0A23C"; border.width: 1.5; border.color: "#4E3A22"; x: parent.width*0.14; y: parent.width*0.14 }
                Rectangle { width: parent.width*0.40; height: parent.width*0.05; radius: 2; color: "#4E3A22"; x: parent.width*0.30; y: parent.width*0.20; rotation: -8 }
                Rectangle { width: parent.width*0.40; height: parent.width*0.05; radius: 2; color: "#4E3A22"; x: parent.width*0.30; y: parent.width*0.25; rotation: 8 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.33; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.56; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.14; height: parent.width*0.10; radius: parent.width*0.03; color: "#F7D9A8"; x: parent.width*0.36; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.12; height: parent.width*0.08; radius: parent.width*0.03; color: "#F7D9A8"; x: parent.width*0.52; y: parent.width*0.56 }
                Text { text: "王"; font.pixelSize: parent.width*0.13; font.bold: true; color: "#4E3A22"; x: parent.width*0.43; y: parent.width*0.21 }
            }
            // ---- 兔 ----
            Item {
                visible: root.petStyle === 3
                anchors.fill: parent
                Rectangle { id: rabbitEarL; width: parent.width*0.16; height: parent.width*0.42; radius: parent.width*0.08; color: "#F4E9DC"; border.width: 1; border.color: "#9A8A78"; x: parent.width*0.30; y: parent.width*0.0 }
                Rectangle { id: rabbitEarR; width: parent.width*0.16; height: parent.width*0.42; radius: parent.width*0.08; color: "#F4E9DC"; border.width: 1; border.color: "#9A8A78"; x: parent.width*0.54; y: parent.width*0.0 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.66; radius: parent.width*0.33; color: "#FBF4EC"; border.width: 1.5; border.color: "#9A8A78"; x: parent.width*0.14; y: parent.width*0.20 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.35; y: parent.width*0.36 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.55; y: parent.width*0.36 }
                Rectangle { width: parent.width*0.10; height: parent.width*0.07; radius: parent.width*0.05; color: "#F49CA0"; x: parent.width*0.45; y: parent.width*0.52 }
                // 兔耳摆动动画
                SequentialAnimation {
                    running: root.petStyle === 3
                    loops: Animation.Infinite
                    NumberAnimation { target: rabbitEarL; property: "rotation"; to: 8; duration: 600; easing.type: Easing.InOutSine }
                    NumberAnimation { target: rabbitEarL; property: "rotation"; to: -8; duration: 600; easing.type: Easing.InOutSine }
                }
            }
            // ---- 龙（大角+眉+长须+鳞纹+龙珠）----
            Item {
                visible: root.petStyle === 4
                anchors.fill: parent
                Rectangle { width: parent.width*0.07; height: parent.width*0.26; radius: 3; color: "#5A7A3A"; x: parent.width*0.29; y: parent.width*0.0; rotation: -10 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.12; radius: 3; color: "#5A7A3A"; x: parent.width*0.27; y: parent.width*0.0; rotation: -45 }
                Rectangle { width: parent.width*0.07; height: parent.width*0.26; radius: 3; color: "#5A7A3A"; x: parent.width*0.64; y: parent.width*0.0; rotation: 10 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.12; radius: 3; color: "#5A7A3A"; x: parent.width*0.67; y: parent.width*0.0; rotation: 45 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.72; radius: parent.width*0.36; color: "#8FC65B"; border.width: 1.5; border.color: "#3E5A28"; x: parent.width*0.14; y: parent.width*0.14 }
                Rectangle { width: parent.width*0.52; height: parent.width*0.05; radius: 2; color: "#6FA640"; x: parent.width*0.24; y: parent.width*0.26 }
                Rectangle { width: parent.width*0.52; height: parent.width*0.05; radius: 2; color: "#6FA640"; x: parent.width*0.24; y: parent.width*0.72 }
                Rectangle { width: parent.width*0.18; height: parent.width*0.06; radius: 3; color: "#3E5A28"; x: parent.width*0.26; y: parent.width*0.27; rotation: -15 }
                Rectangle { width: parent.width*0.18; height: parent.width*0.06; radius: 3; color: "#3E5A28"; x: parent.width*0.56; y: parent.width*0.27; rotation: 15 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.32; y: parent.width*0.36 }
                PetEye { s: parent.width * 0.11; x: parent.width*0.57; y: parent.width*0.36 }
                Rectangle { width: parent.width*0.14; height: parent.width*0.10; radius: parent.width*0.05; color: "#D9534F"; x: parent.width*0.43; y: parent.width*0.52 }
                Rectangle { width: parent.width*0.07; height: parent.width*0.07; radius: parent.width*0.035; color: "#FFD700"; x: parent.width*0.465; y: parent.width*0.46 }
                Rectangle { width: parent.width*0.36; height: 1.5; color: "#3E5A28"; x: parent.width*0.32; y: parent.width*0.61; rotation: -16 }
                Rectangle { width: parent.width*0.36; height: 1.5; color: "#3E5A28"; x: parent.width*0.32; y: parent.width*0.61; rotation: 16 }
            }
            // ---- 蛇 ----
            Item {
                visible: root.petStyle === 5
                anchors.fill: parent
                transform: Rotation {
                    id: snakeRot
                    origin.x: parent.width / 2
                    origin.y: parent.height / 2
                    angle: 0
                }
                Rectangle { width: parent.width*0.60; height: parent.width*0.30; radius: parent.width*0.15; color: "#7DBE5A"; border.width: 1.5; border.color: "#3E5A28"; x: parent.width*0.20; y: parent.width*0.26 }
                PetEye { s: parent.width * 0.09; x: parent.width*0.30; y: parent.width*0.32 }
                PetEye { s: parent.width * 0.09; x: parent.width*0.46; y: parent.width*0.32 }
                Rectangle { width: parent.width*0.05; height: parent.width*0.14; radius: parent.width*0.03; color: "#D9534F"; x: parent.width*0.48; y: parent.width*0.46; rotation: -25 }
                Rectangle { width: parent.width*0.05; height: parent.width*0.14; radius: parent.width*0.03; color: "#D9534F"; x: parent.width*0.48; y: parent.width*0.46; rotation: 25 }
                // 蛇身扭动
                SequentialAnimation {
                    running: root.petStyle === 5
                    loops: Animation.Infinite
                    NumberAnimation { target: snakeRot; property: "angle"; to: -6; duration: 700; easing.type: Easing.InOutSine }
                    NumberAnimation { target: snakeRot; property: "angle"; to: 6;  duration: 700; easing.type: Easing.InOutSine }
                }
            }
            // ---- 马 ----
            Item {
                visible: root.petStyle === 6
                anchors.fill: parent
                Rectangle { width: parent.width*0.12; height: parent.width*0.34; radius: parent.width*0.06; color: "#8A5A2B"; x: parent.width*0.26; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.12; height: parent.width*0.34; radius: parent.width*0.06; color: "#8A5A2B"; x: parent.width*0.62; y: parent.width*0.04 }
                Rectangle { width: parent.width*0.68; height: parent.width*0.62; radius: parent.width*0.30; color: "#B57B3C"; border.width: 1.5; border.color: "#4E3A22"; x: parent.width*0.16; y: parent.width*0.22 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.36; y: parent.width*0.36 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.56; y: parent.width*0.36 }
                Rectangle { width: parent.width*0.10; height: parent.width*0.08; radius: parent.width*0.03; color: "#5C3A1C"; x: parent.width*0.45; y: parent.width*0.56 }
            }
            // ---- 羊 ----
            Item {
                visible: root.petStyle === 7
                anchors.fill: parent
                Rectangle { width: parent.width*0.16; height: parent.width*0.20; radius: parent.width*0.08; color: "#D8C8A8"; x: parent.width*0.20; y: parent.width*0.02; rotation: -20 }
                Rectangle { width: parent.width*0.16; height: parent.width*0.20; radius: parent.width*0.08; color: "#D8C8A8"; x: parent.width*0.64; y: parent.width*0.02; rotation: 20 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.70; radius: parent.width*0.34; color: "#F0E6D2"; border.width: 1.5; border.color: "#8A7A60"; x: parent.width*0.14; y: parent.width*0.16 }
                Rectangle { width: parent.width*0.18; height: parent.width*0.16; radius: parent.width*0.09; color: "#8A7A60"; x: parent.width*0.41; y: parent.width*0.42 }
                PetEye { s: parent.width * 0.09; x: parent.width*0.32; y: parent.width*0.32 }
                PetEye { s: parent.width * 0.09; x: parent.width*0.58; y: parent.width*0.32 }
                // 卷毛
                Rectangle { width: parent.width*0.14; height: parent.width*0.14; radius: parent.width*0.07; color: "#FFFFFF"; x: parent.width*0.10; y: parent.width*0.24 }
                Rectangle { width: parent.width*0.14; height: parent.width*0.14; radius: parent.width*0.07; color: "#FFFFFF"; x: parent.width*0.76; y: parent.width*0.24 }
                Rectangle { width: parent.width*0.16; height: parent.width*0.14; radius: parent.width*0.07; color: "#FFFFFF"; x: parent.width*0.42; y: parent.width*0.04 }
            }
            // ---- 猴 ----
            Item {
                visible: root.petStyle === 8
                anchors.fill: parent
                Rectangle { width: parent.width*0.26; height: parent.width*0.26; radius: parent.width*0.13; color: "#9A6B3F"; x: parent.width*0.06; y: parent.width*0.10 }
                Rectangle { width: parent.width*0.26; height: parent.width*0.26; radius: parent.width*0.13; color: "#9A6B3F"; x: parent.width*0.68; y: parent.width*0.10 }
                Rectangle { width: parent.width*0.70; height: parent.width*0.70; radius: parent.width*0.33; color: "#B07B45"; border.width: 1.5; border.color: "#4E3A22"; x: parent.width*0.15; y: parent.width*0.15 }
                Rectangle { width: parent.width*0.46; height: parent.width*0.42; radius: parent.width*0.20; color: "#E8C9A0"; x: parent.width*0.27; y: parent.width*0.22 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.33; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.57; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.05; radius: parent.width*0.03; color: "#8A6A45"; x: parent.width*0.47; y: parent.width*0.52 }
            }
            // ---- 鸡 ----
            Item {
                visible: root.petStyle === 9
                anchors.fill: parent
                Rectangle { width: parent.width*0.30; height: parent.width*0.14; radius: parent.width*0.04; color: "#D94040"; x: parent.width*0.35; y: parent.width*0.02 }
                Rectangle { width: parent.width*0.20; height: parent.width*0.10; radius: parent.width*0.05; color: "#D94040"; x: parent.width*0.24; y: parent.width*0.08; rotation: -25 }
                Rectangle { width: parent.width*0.20; height: parent.width*0.10; radius: parent.width*0.05; color: "#D94040"; x: parent.width*0.56; y: parent.width*0.08; rotation: 25 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.66; radius: parent.width*0.33; color: "#F2C94C"; border.width: 1.5; border.color: "#8A6A1C"; x: parent.width*0.14; y: parent.width*0.20 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.35; y: parent.width*0.36 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.55; y: parent.width*0.36 }
                Rectangle { width: parent.width*0.10; height: parent.width*0.08; radius: parent.width*0.03; color: "#E07A2A"; x: parent.width*0.45; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.06; radius: parent.width*0.03; color: "#FFFFFF"; x: parent.width*0.24; y: parent.width*0.46 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.06; radius: parent.width*0.03; color: "#FFFFFF"; x: parent.width*0.70; y: parent.width*0.46 }
            }
            // ---- 狗 ----
            Item {
                visible: root.petStyle === 10
                anchors.fill: parent
                Rectangle { width: parent.width*0.20; height: parent.width*0.34; radius: parent.width*0.10; color: "#A97A45"; x: parent.width*0.16; y: parent.width*0.12 }
                Rectangle { width: parent.width*0.20; height: parent.width*0.34; radius: parent.width*0.10; color: "#A97A45"; x: parent.width*0.64; y: parent.width*0.12 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.66; radius: parent.width*0.33; color: "#C99A5E"; border.width: 1.5; border.color: "#4E3A22"; x: parent.width*0.14; y: parent.width*0.20 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.34; y: parent.width*0.36 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.56; y: parent.width*0.36 }
                Rectangle { width: parent.width*0.14; height: parent.width*0.10; radius: parent.width*0.05; color: "#3A2712"; x: parent.width*0.43; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.08; height: parent.width*0.06; radius: parent.width*0.03; color: "#F49CA0"; x: parent.width*0.46; y: parent.width*0.62 }
                // 狗尾巴摆动（在右下）
                Rectangle { id: dogTail; width: parent.width*0.10; height: parent.width*0.18; radius: parent.width*0.05; color: "#A97A45"; x: parent.width*0.80; y: parent.width*0.72 }
                SequentialAnimation {
                    running: root.petStyle === 10
                    loops: Animation.Infinite
                    NumberAnimation { target: dogTail; property: "rotation"; to: 20; duration: 500; easing.type: Easing.InOutSine }
                    NumberAnimation { target: dogTail; property: "rotation"; to: -10; duration: 500; easing.type: Easing.InOutSine }
                }
            }
            // ---- 猪 ----
            Item {
                visible: root.petStyle === 11
                anchors.fill: parent
                Rectangle { width: parent.width*0.16; height: parent.width*0.12; radius: parent.width*0.06; color: "#F0A0B0"; x: parent.width*0.20; y: parent.width*0.10 }
                Rectangle { width: parent.width*0.16; height: parent.width*0.12; radius: parent.width*0.06; color: "#F0A0B0"; x: parent.width*0.64; y: parent.width*0.10 }
                Rectangle { width: parent.width*0.72; height: parent.width*0.70; radius: parent.width*0.35; color: "#F8B4C0"; border.width: 1.5; border.color: "#9A5A6A"; x: parent.width*0.14; y: parent.width*0.14 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.34; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.10; x: parent.width*0.56; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.22; height: parent.width*0.16; radius: parent.width*0.08; color: "#F49CA0"; x: parent.width*0.39; y: parent.width*0.54 }
                Rectangle { width: parent.width*0.04; height: parent.width*0.06; radius: 2; color: "#9A5A6A"; x: parent.width*0.45; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.04; height: parent.width*0.06; radius: 2; color: "#9A5A6A"; x: parent.width*0.51; y: parent.width*0.56 }
                Rectangle { width: parent.width*0.08; height: parent.width*0.06; radius: parent.width*0.04; color: "#F49CA0"; x: parent.width*0.24; y: parent.width*0.48 }
                Rectangle { width: parent.width*0.08; height: parent.width*0.06; radius: parent.width*0.04; color: "#F49CA0"; x: parent.width*0.68; y: parent.width*0.48 }
            }
            // ---- 鱼 ----
            Item {
                visible: root.petStyle === 12
                anchors.fill: parent
                // 整体游动
                SequentialAnimation on x {
                    running: root.petStyle === 12
                    loops: Animation.Infinite
                    NumberAnimation { to: -5; duration: 900; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 5;  duration: 900; easing.type: Easing.InOutSine }
                }
                // 身体
                Rectangle { width: parent.width*0.60; height: parent.width*0.42; radius: parent.width*0.20; color: "#5BA8E8"; border.width: 1.5; border.color: "#2A5A8A"; x: parent.width*0.16; y: parent.width*0.28 }
                // 尾巴
                Rectangle { id: fishTail; width: parent.width*0.20; height: parent.width*0.24; radius: parent.width*0.05; color: "#4A8CC8"; x: parent.width*0.68; y: parent.width*0.34 }
                SequentialAnimation {
                    running: root.petStyle === 12
                    loops: Animation.Infinite
                    NumberAnimation { target: fishTail; property: "rotation"; to: 25; duration: 400; easing.type: Easing.InOutSine }
                    NumberAnimation { target: fishTail; property: "rotation"; to: -25; duration: 400; easing.type: Easing.InOutSine }
                }
                // 背鳍
                Rectangle { width: parent.width*0.14; height: parent.width*0.12; radius: parent.width*0.06; color: "#4A8CC8"; x: parent.width*0.36; y: parent.width*0.20 }
                PetEye { s: parent.width * 0.09; x: parent.width*0.26; y: parent.width*0.36 }
                // 泡泡
                Rectangle { width: parent.width*0.06; height: parent.width*0.06; radius: parent.width*0.03; color: "#BBD8F0"; border.width: 0.5; border.color: "#7AA0C0"; x: parent.width*0.10; y: parent.width*0.10 }
                Rectangle { width: parent.width*0.04; height: parent.width*0.04; radius: parent.width*0.02; color: "#BBD8F0"; x: parent.width*0.06; y: parent.width*0.18 }
            }
            // ---- 猫（橘猫，尖耳+胡须+条纹）----
            Item {
                visible: root.petStyle === 13
                anchors.fill: parent
                Rectangle { width: parent.width*0.18; height: parent.width*0.24; color: "#E8833A"; x: parent.width*0.14; y: parent.width*0.02; rotation: -16 }
                Rectangle { width: parent.width*0.08; height: parent.width*0.12; color: "#F8C8B0"; x: parent.width*0.17; y: parent.width*0.06; rotation: -16 }
                Rectangle { width: parent.width*0.18; height: parent.width*0.24; color: "#E8833A"; x: parent.width*0.68; y: parent.width*0.02; rotation: 16 }
                Rectangle { width: parent.width*0.08; height: parent.width*0.12; color: "#F8C8B0"; x: parent.width*0.75; y: parent.width*0.06; rotation: 16 }
                Rectangle { width: parent.width*0.70; height: parent.width*0.66; radius: parent.width*0.33; color: "#F0A050"; border.width: 1.5; border.color: "#8A4A20"; x: parent.width*0.15; y: parent.width*0.16 }
                Rectangle { width: parent.width*0.26; height: parent.width*0.07; radius: 3; color: "#C96A28"; x: parent.width*0.37; y: parent.width*0.22; rotation: -6 }
                PetEye { s: parent.width * 0.12; x: parent.width*0.31; y: parent.width*0.34 }
                PetEye { s: parent.width * 0.12; x: parent.width*0.57; y: parent.width*0.34 }
                Rectangle { width: parent.width*0.09; height: parent.width*0.07; radius: parent.width*0.035; color: "#F06070"; x: parent.width*0.455; y: parent.width*0.52 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.05; radius: parent.width*0.03; color: "#8A4A20"; x: parent.width*0.44; y: parent.width*0.57 }
                Rectangle { width: parent.width*0.06; height: parent.width*0.05; radius: parent.width*0.03; color: "#8A4A20"; x: parent.width*0.50; y: parent.width*0.57 }
                Rectangle { width: parent.width*0.20; height: 1.5; color: "#8A6A4A"; x: parent.width*0.12; y: parent.width*0.50; rotation: 12 }
                Rectangle { width: parent.width*0.20; height: 1.5; color: "#8A6A4A"; x: parent.width*0.68; y: parent.width*0.50; rotation: -12 }
                Rectangle { width: parent.width*0.18; height: 1.5; color: "#8A6A4A"; x: parent.width*0.13; y: parent.width*0.60; rotation: -10 }
                Rectangle { width: parent.width*0.18; height: 1.5; color: "#8A6A4A"; x: parent.width*0.69; y: parent.width*0.60; rotation: 10 }
                Rectangle { id: catTail; width: parent.width*0.09; height: parent.width*0.24; radius: parent.width*0.045; color: "#E8833A"; x: parent.width*0.80; y: parent.width*0.68 }
                SequentialAnimation {
                    running: root.petStyle === 13
                    loops: Animation.Infinite
                    NumberAnimation { target: catTail; property: "rotation"; to: 25; duration: 600; easing.type: Easing.InOutSine }
                    NumberAnimation { target: catTail; property: "rotation"; to: -15; duration: 600; easing.type: Easing.InOutSine }
                }
            }
        }
    }

    // ============ 关于弹窗 ============
    function showAbout() {
        positionPopup(aboutPopup)
        aboutPopup.open()
    }

    PanelPopup {
        id: aboutPopup
        width: 340
        height: 350
        popupX: DockPanelPositioner.x
        popupY: DockPanelPositioner.y

        Rectangle {
            anchors.fill: parent
            radius: 14
            color: root.popupBg
            border.width: 1
            border.color: root.popupBorder

            Column {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                Text {
                    width: parent.width
                    text: "👀 关于"
                    font.pixelSize: 20
                    font.bold: true
                    color: root.textMain
                    horizontalAlignment: Text.AlignHCenter
                }
                Rectangle { width: parent.width; height: 1; color: root.dividerColor }
                Text {
                    width: parent.width
                    text: "作者：chnome"
                    font.pixelSize: 14
                    font.bold: true
                    color: root.accentColor
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    width: parent.width
                    text: "任务栏卡通眼珠插件"
                    font.pixelSize: 13
                    color: root.textBody
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    width: parent.width
                    text: "· 多主题眼睛 / 宠物模式（十二生肖·鱼·猫）\n· 点击弹出学习内容：诗经 / 唐诗宋词 / 英语词汇 / 维基百科\n· 含注释例句、网络语音朗读、一键复制\n· 瞳孔跟随参照 GEyes（gnome-applets）算法"
                    font.pixelSize: 12
                    color: root.textSub
                    lineHeight: 1.6
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }
                Rectangle {
                    width: 90
                    height: 30
                    radius: 15
                    color: root.btnBg
                    border.width: 1
                    border.color: root.popupBorder
                    anchors.horizontalCenter: parent.horizontalCenter
                    Text {
                        anchors.centerIn: parent
                        text: "知道了"
                        font.pixelSize: 13
                        color: root.accentColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: aboutPopup.close()
                    }
                }
            }
        }
    }

    // ============ 交互：左键学习 / 右键菜单 ============
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        preventStealing: true
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                root.showEyeMenu()
            } else {
                root.showLearning()
            }
            mouse.accepted = true
        }
        onPressed: {
            mouse.accepted = true
        }
    }

    // ============ 右键菜单（PanelPopup 自定义两级菜单） ============
    // 第一级：眼睛样式 / 学习内容；点击进入对应第二级列表
    property int menuLevel: 0      // 0=根菜单, 1=眼睛样式, 2=学习内容
    // 分类勾选本地状态（Q_INVOKABLE 方法绑定不自动刷新，用本地数组驱动）
    property var catState: [true, true, true, true, true, true, true, true, true, true]

    function refreshCatState() {
        var arr = []
        for (var i = 0; i < catState.length; i++) {
            arr.push(Applet.categoryEnabled(i))
        }
        catState = arr   // 整体赋值，触发 checked 绑定刷新
    }

    function toggleCategory(i) {
        var arr = catState.slice()
        arr[i] = !arr[i]
        catState = arr   // 整体赋值，触发 checked 绑定刷新
        Applet.setCategoryEnabled(i, arr[i])
    }

    // 根据任务栏位置 + 当前菜单高度重定位（底部任务栏时向上按高度展开）
    function repositionEyeMenu() {
        var pt = root.mapToItem(null, 0, 0)
        if (Panel.position === Dock.Top) {
            eyeMenu.menuX = pt.x
            eyeMenu.menuY = pt.y + root.height + 10
        } else if (Panel.position === Dock.Left) {
            eyeMenu.menuX = pt.x + root.width + 10
            eyeMenu.menuY = pt.y
        } else if (Panel.position === Dock.Right) {
            eyeMenu.menuX = pt.x - eyeMenu.width - 10
            eyeMenu.menuY = pt.y
        } else {
            eyeMenu.menuX = pt.x
            eyeMenu.menuY = pt.y - eyeMenu.height - 10
        }
    }

    function showEyeMenu() {
        menuLevel = 0
        refreshCatState()
        repositionEyeMenu()
        eyeMenu.open()
    }

    function gotoMenuLevel(lv) {
        menuLevel = lv
        if (menuFlick) menuFlick.contentY = 0
        // 根菜单（设置）固定 225px；二级菜单等下一帧内容布局完成后按实际高度自适应
        Qt.callLater(function() {
            if (root.menuLevel === 0) {
                eyeMenu.menuH = 225
            } else {
                eyeMenu.menuH = Math.max(100, menuCol.implicitHeight + 56)
            }
            if (eyeMenu.popupVisible) {
                root.repositionEyeMenu()
            }
        })
    }

    // 菜单行组件
    component MenuRow: Rectangle {
        property string label: ""
        property bool checked: false
        property bool showArrow: false
        property bool isHeader: false
        property bool hovered: false
        width: parent.width
        height: isHeader ? 24 : 30
        radius: 6
        color: isHeader ? "transparent" : (hovered ? (root.darkMode ? "#3A3E45" : "#E9EEF5") : "transparent")
        signal clicked()

        Text {
            anchors.left: parent.left
            anchors.leftMargin: isHeader ? 10 : 14
            anchors.verticalCenter: parent.verticalCenter
            text: label
            font.pixelSize: isHeader ? 11 : 13
            font.bold: isHeader
            color: isHeader ? root.accentColor : root.textMain
            elide: Text.ElideRight
            width: parent.width - 50
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: !isHeader
            text: showArrow ? "▸" : (checked ? "✓" : "")
            font.pixelSize: 13
            color: root.accentColor
        }
        MouseArea {
            anchors.fill: parent
            enabled: !isHeader
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            onEntered: parent.hovered = true
            onExited: parent.hovered = false
            onClicked: parent.clicked()
        }
    }

    // ============ 右键菜单（PanelMenu 两级菜单） ============
    PanelMenu {
        id: eyeMenu
        property int menuH: 225
        height: menuH
        onHeightChanged: {
            if (popupVisible) {
                root.repositionEyeMenu()
            }
        }
        menuX: 0
        menuY: -400
        windowTitle: "dde-shell/eye-menu"

        // 内容（宽 250，高度跟随窗口）
        Rectangle {
            width: 250
            height: parent.height
            radius: 12
            color: root.popupBg
            border.width: 1
            border.color: root.popupBorder

            // 顶栏：返回 / 标题 / 关闭
            RowLayout {
                id: menuHeader
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                height: 30
                spacing: 6

                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 24
                    radius: 12
                    visible: root.menuLevel > 0
                    color: root.btnBg
                    Text {
                        anchors.centerIn: parent
                        text: "‹"
                        font.pixelSize: 16
                        color: root.textMain
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: root.gotoMenuLevel(0)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: root.menuLevel === 0 ? qsTr("设置") : (root.menuLevel === 1 ? qsTr("眼睛样式") : (root.menuLevel === 3 ? qsTr("宠物样式") : (root.menuLevel === 4 ? qsTr("显示位置") : qsTr("学习内容"))))
                    font.pixelSize: 13
                    font.bold: true
                    color: root.textMain
                    horizontalAlignment: Text.AlignHCenter
                }
                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 24
                    radius: 12
                    color: root.btnBg
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: root.textSub
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onEntered: parent.color = root.darkMode ? "#41454D" : "#DCE4EE"
                        onExited: parent.color = root.btnBg
                        onClicked: eyeMenu.close()
                    }
                }
            }

            // 内容区（可滚动）
            Flickable {
                id: menuFlick
                anchors.top: menuHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                clip: true
                contentHeight: menuCol.implicitHeight

                Column {
                    id: menuCol
                    width: parent.width
                    spacing: 2

                    Column {
                        visible: root.menuLevel === 0
                        width: parent.width
                        spacing: 2
                        MenuRow { label: qsTr("眼睛样式"); showArrow: true; onClicked: root.gotoMenuLevel(1) }
                        MenuRow { label: qsTr("宠物样式"); showArrow: true; onClicked: root.gotoMenuLevel(3) }
                        MenuRow { label: qsTr("学习内容"); showArrow: true; onClicked: root.gotoMenuLevel(2) }
                        MenuRow { label: qsTr("显示位置"); showArrow: true; onClicked: root.gotoMenuLevel(4) }
                        MenuRow { label: qsTr("关于"); onClicked: { eyeMenu.close(); Qt.callLater(root.showAbout) } }
                    }

                    Column {
                        visible: root.menuLevel === 1
                        width: parent.width
                        spacing: 2
                        MenuRow { label: qsTr("蓝色卡通 (Default)") ;        checked: root.eyeStyle === 0 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 0; eyeMenu.close() } }
                        MenuRow { label: qsTr("双眼睛 (Bizarre)") ;          checked: root.eyeStyle === 1 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 1; eyeMenu.close() } }
                        MenuRow { label: qsTr("绿眸少女 (Green-EyedGirl)");  checked: root.eyeStyle === 2 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 2; eyeMenu.close() } }
                        MenuRow { label: qsTr("粉眸少女 (Pink-EyedGirl)");   checked: root.eyeStyle === 3 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 3; eyeMenu.close() } }
                        MenuRow { label: qsTr("浓睫大眼 (EyelashLarge)");    checked: root.eyeStyle === 4 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 4; eyeMenu.close() } }
                        MenuRow { label: qsTr("恶魔红眼 (Bloodshot)");       checked: root.eyeStyle === 5 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 5; eyeMenu.close() } }
                        MenuRow { label: qsTr("恐怖之眼 (Horrid)");          checked: root.eyeStyle === 6 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 6; eyeMenu.close() } }
                        MenuRow { label: qsTr("南瓜怪眼 (PumpkinMonster)");  checked: root.eyeStyle === 7 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 7; eyeMenu.close() } }
                        MenuRow { label: qsTr("奇异双瞳 (Bizarre-2)");       checked: root.eyeStyle === 8 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 8; eyeMenu.close() } }
                        MenuRow { label: qsTr("猫瞳 (Cat)");                 checked: root.eyeStyle === 9 && root.petStyle < 0;  onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 9; eyeMenu.close() } }
                        MenuRow { label: qsTr("哆啦A梦");                    checked: root.eyeStyle === 10 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 10; eyeMenu.close() } }
                        MenuRow { label: qsTr("圆眼双瞳");                   checked: root.eyeStyle === 11 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 11; eyeMenu.close() } }
                        MenuRow { label: qsTr("蓝色卡通·双眼");                checked: root.eyeStyle === 12 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 12; eyeMenu.close() } }
                        MenuRow { label: qsTr("绿眸少女·双眼");                checked: root.eyeStyle === 13 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 13; eyeMenu.close() } }
                        MenuRow { label: qsTr("粉眸少女·双眼");                checked: root.eyeStyle === 14 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 14; eyeMenu.close() } }
                        MenuRow { label: qsTr("浓睫大眼·双眼");                checked: root.eyeStyle === 15 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 15; eyeMenu.close() } }
                        MenuRow { label: qsTr("恶魔红眼·双眼");                checked: root.eyeStyle === 16 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 16; eyeMenu.close() } }
                        MenuRow { label: qsTr("恐怖之眼·双眼");                checked: root.eyeStyle === 17 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 17; eyeMenu.close() } }
                        MenuRow { label: qsTr("南瓜怪眼·双眼");                checked: root.eyeStyle === 18 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 18; eyeMenu.close() } }
                        MenuRow { label: qsTr("猫瞳·双眼");                    checked: root.eyeStyle === 19 && root.petStyle < 0; onClicked: { Applet.petStyle = -1; Applet.eyeStyle = 19; eyeMenu.close() } }
                    }

                    Column {
                        visible: root.menuLevel === 4
                        width: parent.width
                        spacing: 2
                        MenuRow { label: qsTr("左侧"); checked: root.posPref === 0; onClicked: { Applet.posPref = 0; eyeMenu.close() } }
                        MenuRow { label: qsTr("居中（启动器右侧）"); checked: root.posPref === 1; onClicked: { Applet.posPref = 1; eyeMenu.close() } }
                        MenuRow { label: qsTr("右侧"); checked: root.posPref === 2; onClicked: { Applet.posPref = 2; eyeMenu.close() } }
                    }

                    Column {
                        visible: root.menuLevel === 3
                        width: parent.width
                        spacing: 2
                        MenuRow { label: qsTr("眼睛模式（关闭宠物）"); checked: root.petStyle < 0; onClicked: { Applet.petStyle = -1; eyeMenu.close() } }
                        MenuRow { label: qsTr("鼠 🐭"); checked: root.petStyle === 0;  onClicked: { Applet.petStyle = 0;  eyeMenu.close() } }
                        MenuRow { label: qsTr("牛 🐮"); checked: root.petStyle === 1;  onClicked: { Applet.petStyle = 1;  eyeMenu.close() } }
                        MenuRow { label: qsTr("虎 🐯"); checked: root.petStyle === 2;  onClicked: { Applet.petStyle = 2;  eyeMenu.close() } }
                        MenuRow { label: qsTr("兔 🐰"); checked: root.petStyle === 3;  onClicked: { Applet.petStyle = 3;  eyeMenu.close() } }
                        MenuRow { label: qsTr("龙 🐲"); checked: root.petStyle === 4;  onClicked: { Applet.petStyle = 4;  eyeMenu.close() } }
                        MenuRow { label: qsTr("蛇 🐍"); checked: root.petStyle === 5;  onClicked: { Applet.petStyle = 5;  eyeMenu.close() } }
                        MenuRow { label: qsTr("马 🐴"); checked: root.petStyle === 6;  onClicked: { Applet.petStyle = 6;  eyeMenu.close() } }
                        MenuRow { label: qsTr("羊 🐑"); checked: root.petStyle === 7;  onClicked: { Applet.petStyle = 7;  eyeMenu.close() } }
                        MenuRow { label: qsTr("猴 🐵"); checked: root.petStyle === 8;  onClicked: { Applet.petStyle = 8;  eyeMenu.close() } }
                        MenuRow { label: qsTr("鸡 🐔"); checked: root.petStyle === 9;  onClicked: { Applet.petStyle = 9;  eyeMenu.close() } }
                        MenuRow { label: qsTr("狗 🐶"); checked: root.petStyle === 10; onClicked: { Applet.petStyle = 10; eyeMenu.close() } }
                        MenuRow { label: qsTr("猪 🐷"); checked: root.petStyle === 11; onClicked: { Applet.petStyle = 11; eyeMenu.close() } }
                        MenuRow { label: qsTr("鱼 🐟"); checked: root.petStyle === 12; onClicked: { Applet.petStyle = 12; eyeMenu.close() } }
                        MenuRow { label: qsTr("猫 🐱"); checked: root.petStyle === 13; onClicked: { Applet.petStyle = 13; eyeMenu.close() } }
                    }

                    Column {
                        visible: root.menuLevel === 2
                        width: parent.width
                        spacing: 2
                        MenuRow { label: qsTr("英文学习"); isHeader: true }
                        MenuRow { label: qsTr("雅思词汇（含注释和例句）");        checked: root.catState[3]; onClicked: root.toggleCategory(3) }
                        MenuRow { label: qsTr("托福词汇（含注释和例句）");        checked: root.catState[4]; onClicked: root.toggleCategory(4) }
                        MenuRow { label: qsTr("大学四级词汇（含注释和例句）");    checked: root.catState[5]; onClicked: root.toggleCategory(5) }
                        MenuRow { label: qsTr("大学六级词汇（含注释和例句）");    checked: root.catState[6]; onClicked: root.toggleCategory(6) }
                        MenuRow { label: qsTr("高考核心词汇（含注释和例句）");    checked: root.catState[7]; onClicked: root.toggleCategory(7) }
                        MenuRow { label: qsTr("常用英语2000词（含注释和例句）"); checked: root.catState[8]; onClicked: root.toggleCategory(8) }
                        MenuRow { label: qsTr("诗歌学习"); isHeader: true }
                        MenuRow { label: qsTr("唐诗三百首（含注释）");            checked: root.catState[0]; onClicked: root.toggleCategory(0) }
                        MenuRow { label: qsTr("宋词三百首（含注释）");            checked: root.catState[1]; onClicked: root.toggleCategory(1) }
                        MenuRow { label: qsTr("诗经（含注释）");                  checked: root.catState[2]; onClicked: root.toggleCategory(2) }
                        MenuRow { label: qsTr("百科学习"); isHeader: true }
                        MenuRow { label: qsTr("维基百科精选词条（含简介）");        checked: root.catState[9]; onClicked: root.toggleCategory(9) }
                    }
                }
            }
        }
    }
}

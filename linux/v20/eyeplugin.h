// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// deepin V20 (dde-dock 5.x) 版卡通眼珠插件。
//
// 与 V25 (dde-shell) 版功能对齐：多主题眼睛 / 宠物模式 / 学习内容弹窗 /
// 两级设置菜单 / 网络 TTS / 深浅色外观。
// 接口使用 dde-dock 的 PluginsItemInterface（V1，
// IID = com.deepin.dock.PluginsItemInterface），非 dde-tray-loader 的 V2。

#pragma once

#include "pluginsiteminterface.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QPoint>
#include <QProcess>
#include <QSettings>
#include <QVariantMap>
#include <QVector>

class QWidget;
class QTimer;
class EyeWidget;

class EyePlugin : public QObject, public PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "plugin.json")

public:
    explicit EyePlugin(QObject *parent = nullptr);
    ~EyePlugin() override;

    // ============ PluginsItemInterface ============
    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    const QString itemCommand(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId,
                         const bool checked) override;
    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;
    bool pluginIsAllowDisable() override;
    bool pluginIsDisable() override;
    void pluginStateSwitched() override;
    void refreshIcon(const QString &itemKey) override;
    void positionChanged(const Dock::Position position) override;
    void displayModeChanged(const Dock::DisplayMode displayMode) override;

    // ============ 状态 ============
    int eyeStyle() const { return m_eyeStyle; }
    void setEyeStyle(int s);
    int petStyle() const { return m_petStyle; }
    void setPetStyle(int s);
    int posPref() const { return m_posPref; }
    void setPosPref(int p);

    bool categoryEnabled(int idx) const;
    void setCategoryEnabled(int idx, bool en);

    // 当前任务栏停靠位置（Dock::Top / Right / Bottom / Left）。
    // 优先读 dde-dock 写在 QApplication 动态属性上的实时值，
    // 拿不到时退回 positionChanged() 缓存的 m_dockPos。
    Dock::Position dockPosition() const;
    QPoint cursorPos() const { return m_cursorPos; }

    // ============ 学习内容 ============
    bool currentEmpty() const { return m_learnEmpty; }
    QVariantMap currentItem() const { return m_learnItem; }
    void pickNextLearningItem();
    QString currentPlainText() const;
    QString currentLearnHtml() const;
    QString aboutHtml() const;
    QString chipText() const;

    // ============ TTS / 剪贴板 ============
    void speak(const QString &text, const QString &lang);
    void copyText(const QString &text);

signals:
    // 样式 / 位置 / 内容变化 → 控件与弹窗刷新
    void styleChanged();
    void learningChanged();

private slots:
    void pollCursor();

private:
    bool loadCategories();
    void fetchAndPlay(const QString &url, const QString &text, bool enFallback);
    void playMp3(const QByteArray &data);
    static bool looksLikeMp3(const QByteArray &data);

    PluginProxyInterface *m_proxyInter = nullptr;
    EyeWidget *m_widget = nullptr;

    QSettings m_settings;
    int m_eyeStyle = 0;
    int m_petStyle = -1;    // -1 = 眼睛模式
    int m_posPref = 0;      // 0 左 / 1 中 / 2 右
    Dock::Position m_dockPos = Dock::Bottom;
    QPoint m_cursorPos;
    QTimer *m_cursorTimer = nullptr;

    // 学习数据
    QVector<QVector<QVariantMap>> m_categories;
    QVector<QVector<int>> m_shown;
    QStringList m_catNames;
    QStringList m_catFiles;
    QVariantMap m_learnItem;
    bool m_learnEmpty = false;

    QNetworkAccessManager m_net;
    QProcess m_ttsProcess;
};

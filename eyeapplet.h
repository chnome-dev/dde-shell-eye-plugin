// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <applet.h>
#include <dock/dappletdock.h>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPoint>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

DS_USE_NAMESPACE

/**
 * 任务栏卡通眼珠 Applet 后端
 * - 继承 DAppletDock 以便在系统"插件区域"中显示并可勾选显隐
 * - 轮询鼠标位置、保存眼睛样式
 * - 加载 10 类学习数据（唐诗/宋词/诗经/雅思/托福/四六级/高考/常用2000词/维基百科），
 *   按加权随机弹出（已出现的降低频率），并通过网络 TTS 朗读
 */
class EyeApplet : public DAppletDock
{
    Q_OBJECT
    Q_PROPERTY(int cursorX READ cursorX NOTIFY cursorPosChanged)
    Q_PROPERTY(int cursorY READ cursorY NOTIFY cursorPosChanged)
    Q_PROPERTY(int eyeStyle READ eyeStyle WRITE setEyeStyle NOTIFY eyeStyleChanged)
    Q_PROPERTY(int petStyle READ petStyle WRITE setPetStyle NOTIFY petStyleChanged)
    Q_PROPERTY(int posPref READ posPref WRITE setPosPref NOTIFY posPrefChanged)

public:
    explicit EyeApplet(QObject *parent = nullptr);
    ~EyeApplet() override;

    bool load() override;
    bool init() override;

    int cursorX() const;
    int cursorY() const;

    int eyeStyle() const;
    void setEyeStyle(int style);

    int petStyle() const;
    void setPetStyle(int style);

    int posPref() const;   // 0=左侧 1=居中 2=右侧
    void setPosPref(int p);

    DockItemInfo dockItemInfo() override;

    // ============ 学习内容 ============
    Q_INVOKABLE QVariantMap nextItem();                     // 加权随机取一条学习内容
    Q_INVOKABLE bool categoryEnabled(int idx) const;        // 分类是否勾选
    Q_INVOKABLE void setCategoryEnabled(int idx, bool en);  // 勾选/取消分类
    Q_INVOKABLE void speak(const QString &text, const QString &lang); // 网络TTS朗读
    Q_INVOKABLE void copyText(const QString &text);                  // 复制文本到剪贴板

private:
    void fetchAndPlay(const QString &url, const QString &text, bool enFallback);
    void playMp3(const QByteArray &data);
    bool looksLikeMp3(const QByteArray &data) const;

signals:
    void cursorPosChanged();
    void eyeStyleChanged();
    void petStyleChanged();
    void posPrefChanged();

private:
    bool loadCategories();

    QTimer m_timer;
    QPoint m_cursorPos;
    int m_eyeStyle = 0;
    int m_petStyle = -1;   // -1=眼睛模式，0-13=宠物
    int m_posPref = 0;     // 0=左侧 1=居中 2=右侧
    QSettings m_settings;

    // 学习数据
    QList<QList<QVariantMap>> m_categories;   // 8 个分类的数据
    QList<QVector<int>> m_shown;              // 每个条目的已展示次数（用于降低重复频率）
    QStringList m_catNames;                   // 分类显示名
    QStringList m_catFiles;                   // 数据文件名

    QNetworkAccessManager m_net;
    QProcess m_ttsProcess;
};

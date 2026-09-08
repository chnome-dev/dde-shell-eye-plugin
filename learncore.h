// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QObject>
#include <QSettings>
#include <QVariantMap>
#include <QVector>

class QTextToSpeech;

/**
 * 学习数据核心：加载 10 类 JSON 数据，加权随机取一条，
 * 分类勾选持久化，Windows SAPI 语音朗读。
 * 由 dde-shell 插件的 eyeapplet.cpp 逻辑移植而来（去除 DDE 依赖）。
 */
class LearnCore : public QObject
{
    Q_OBJECT
public:
    enum CategoryType { Poem, Word, Wiki };

    explicit LearnCore(QObject *parent = nullptr);

    bool loadData(const QString &dataDir);   // 加载 data/*.json
    int categoryCount() const { return 10; }
    QString categoryName(int idx) const;
    static CategoryType categoryType(int idx);

    bool categoryEnabled(int idx) const;
    void setCategoryEnabled(int idx, bool en);

    QVariantMap nextItem();                  // 加权随机取一条（出现过的降频）
    QString buildFullText(const QVariantMap &item) const; // 组装完整文本（供复制）

    void speak(const QString &text, const QString &lang); // TTS 朗读

private:
    QList<QString> m_catNames;
    QList<QString> m_catFiles;
    QList<QList<QVariantMap>> m_categories;
    QList<QVector<int>> m_shown;
    QSettings m_settings;
    QTextToSpeech *m_speech = nullptr;
    QString m_speechLang;
};

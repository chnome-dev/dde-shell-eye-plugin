// SPDX-License-Identifier: GPL-3.0-or-later
#include "learncore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRandomGenerator>
#include <QTextToSpeech>

LearnCore::LearnCore(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("CHNOME"), QStringLiteral("CartoonEye"))
{
}

bool LearnCore::loadData(const QString &dataDir)
{
    m_catNames = {
        QStringLiteral("唐诗三百首"), QStringLiteral("宋词三百首"), QStringLiteral("诗经"),
        QStringLiteral("雅思词汇"), QStringLiteral("托福词汇"), QStringLiteral("大学四级词汇"),
        QStringLiteral("大学六级词汇"), QStringLiteral("高考核心词汇"), QStringLiteral("常用英语2000词"),
        QStringLiteral("维基百科精选词条")
    };
    m_catFiles = {
        QStringLiteral("tangshi.json"), QStringLiteral("songci.json"), QStringLiteral("shijing.json"),
        QStringLiteral("ielts.json"), QStringLiteral("toefl.json"), QStringLiteral("cet4.json"),
        QStringLiteral("cet6.json"), QStringLiteral("gaokao.json"), QStringLiteral("c2000.json"),
        QStringLiteral("wiki.json")
    };

    m_categories.clear();
    m_shown.clear();
    QDir dir(dataDir);
    for (int i = 0; i < m_catFiles.size(); ++i) {
        QList<QVariantMap> items;
        QFile f(dir.filePath(m_catFiles.at(i)));
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isArray()) {
                const QJsonArray arr = doc.array();
                for (const auto &v : arr) {
                    if (v.isObject())
                        items.append(v.toObject().toVariantMap());
                }
            }
        }
        m_categories.append(items);
        m_shown.append(QVector<int>(items.size(), 0));
    }
    return !m_categories.isEmpty();
}

QString LearnCore::categoryName(int idx) const
{
    return (idx >= 0 && idx < m_catNames.size()) ? m_catNames.at(idx) : QString();
}

LearnCore::CategoryType LearnCore::categoryType(int idx)
{
    if (idx == 9) return Wiki;
    return (idx <= 2) ? Poem : Word;
}

bool LearnCore::categoryEnabled(int idx) const
{
    if (idx < 0 || idx >= categoryCount()) return false;
    int mask = m_settings.value(QStringLiteral("enabledCategories"), 0x3FF).toInt();
    return (mask & (1 << idx)) != 0;
}

void LearnCore::setCategoryEnabled(int idx, bool en)
{
    if (idx < 0 || idx >= categoryCount()) return;
    int mask = m_settings.value(QStringLiteral("enabledCategories"), 0x3FF).toInt();
    if (en) mask |= (1 << idx);
    else    mask &= ~(1 << idx);
    m_settings.setValue(QStringLiteral("enabledCategories"), mask);
}

QVariantMap LearnCore::nextItem()
{
    QList<int> enabled;
    for (int i = 0; i < categoryCount(); ++i) {
        if (categoryEnabled(i) && !m_categories.at(i).isEmpty())
            enabled.append(i);
    }
    if (enabled.isEmpty()) return QVariantMap();

    const int cat = enabled.at(QRandomGenerator::global()->bounded(enabled.size()));
    const auto &items = m_categories.at(cat);
    const auto &shown = m_shown.at(cat);

    QVector<double> weights(items.size());
    double total = 0.0;
    for (int i = 0; i < items.size(); ++i) {
        weights[i] = 1.0 / (1.0 + 1.2 * shown.at(i));
        total += weights[i];
    }
    double r = QRandomGenerator::global()->generateDouble() * total;
    int idx = items.size() - 1;
    for (int i = 0; i < items.size(); ++i) {
        r -= weights.at(i);
        if (r <= 0.0) { idx = i; break; }
    }
    m_shown[cat][idx] += 1;

    QVariantMap m = items.at(idx);
    m.insert(QStringLiteral("catIndex"), cat);
    m.insert(QStringLiteral("catName"), categoryName(cat));
    const auto t = categoryType(cat);
    m.insert(QStringLiteral("type"), t == Poem ? QStringLiteral("poem")
                                       : (t == Wiki ? QStringLiteral("wiki") : QStringLiteral("word")));
    return m;
}

QString LearnCore::buildFullText(const QVariantMap &item) const
{
    if (item.isEmpty()) return QString();
    const QString type = item.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("poem")) {
        QString s = item.value(QStringLiteral("t")).toString() + QLatin1Char('\n')
                    + item.value(QStringLiteral("a")).toString();
        if (item.contains(QStringLiteral("sec")))
            s += QStringLiteral(" · ") + item.value(QStringLiteral("sec")).toString();
        s += QStringLiteral("\n\n") + item.value(QStringLiteral("txt")).toString();
        if (item.contains(QStringLiteral("trans")))
            s += QStringLiteral("\n\n【译文】\n") + item.value(QStringLiteral("trans")).toString();
        if (item.contains(QStringLiteral("note")))
            s += QStringLiteral("\n\n【赏析·注释】\n") + item.value(QStringLiteral("note")).toString();
        return s;
    }
    if (type == QLatin1String("wiki")) {
        return item.value(QStringLiteral("t")).toString() + QStringLiteral("\n\n")
               + item.value(QStringLiteral("d")).toString() + QStringLiteral("\n\n（来源：维基百科）");
    }
    QString s = item.value(QStringLiteral("w")).toString();
    if (item.contains(QStringLiteral("p")))
        s += QStringLiteral("  /") + item.value(QStringLiteral("p")).toString() + QStringLiteral("/");
    s += QLatin1Char('\n') + item.value(QStringLiteral("d")).toString();
    const auto es = item.value(QStringLiteral("es")).toList();
    for (int i = 0; i < es.size(); ++i) {
        const auto e = es.at(i).toMap();
        s += QStringLiteral("\n\n例句%1: %2").arg(i + 1).arg(e.value(QStringLiteral("e")).toString());
        if (e.contains(QStringLiteral("c")))
            s += QLatin1Char('\n') + e.value(QStringLiteral("c")).toString();
    }
    return s;
}

void LearnCore::speak(const QString &text, const QString &lang)
{
    if (text.trimmed().isEmpty()) return;
    if (!m_speech) m_speech = new QTextToSpeech(this);
    if (m_speechLang != lang) {
        QLocale loc(lang == QLatin1String("zh") ? QLocale::Chinese : QLocale::English);
        m_speech->setLocale(loc);
        m_speechLang = lang;
    }
    m_speech->say(text);
}

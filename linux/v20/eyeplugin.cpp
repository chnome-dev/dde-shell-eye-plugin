// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// deepin V20 (dde-dock) 版卡通眼珠插件实现。

#include "eyeplugin.h"

#include "eyeartist.h"
#include "eyestyle.h"
#include "eyewidget.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr int kCategoryCount = 10;
constexpr const char *kPluginName = "cartoon-eye";
constexpr const char *kDataDir = "/usr/share/dde-dock/cartoon-eye/data";

// 供 QTextBrowser 里的小喇叭链接使用：eye-speak:<lang>|<percent-encoded text>
QString speakPayload(const QString &text, const QString &lang)
{
    return lang + QLatin1Char('|')
           + QString::fromUtf8(QUrl::toPercentEncoding(text.trimmed()));
}
} // namespace

EyePlugin::EyePlugin(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("deepin"), QStringLiteral("dde-dock-cartoon-eye"))
{
    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setInterval(33);
    m_cursorTimer->setTimerType(Qt::PreciseTimer);
    connect(m_cursorTimer, &QTimer::timeout, this, &EyePlugin::pollCursor);
}

EyePlugin::~EyePlugin() = default;

// ===========================================================================
// PluginsItemInterface
// ===========================================================================
const QString EyePlugin::pluginName() const
{
    return QString::fromLatin1(kPluginName);
}

const QString EyePlugin::pluginDisplayName() const
{
    return tr("卡通眼珠");
}

void EyePlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    m_eyeStyle = m_settings.value(QStringLiteral("eyeStyle"), 0).toInt();
    m_petStyle = m_settings.value(QStringLiteral("petStyle"), -1).toInt();
    m_posPref = m_settings.value(QStringLiteral("posPref"), 0).toInt();

    loadCategories();
    m_cursorTimer->start();

    if (!pluginIsDisable())
        m_proxyInter->itemAdded(this, pluginName());
}

QWidget *EyePlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    if (!m_widget)
        m_widget = new EyeWidget(this);
    return m_widget;
}

QWidget *EyePlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return nullptr;
}

const QString EyePlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    return QString();
}

// 右键菜单（dde-dock 原生 JSON 菜单，作为自定义菜单的兜底）
const QString EyePlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QList<QVariant> items;
    auto add = [&items](const QString &id, const QString &text, bool checkable, bool checked) {
        QVariantMap m;
        m["itemId"] = id;
        m["itemText"] = text;
        m["isActive"] = true;
        m["isCheckable"] = checkable;
        m["checked"] = checked;
        m["itemIcon"] = "";
        items.append(m);
    };
    add(QStringLiteral("pos_left"), tr("显示位置：左侧"), true, m_posPref == 0);
    add(QStringLiteral("pos_center"), tr("显示位置：居中（启动器右侧）"), true, m_posPref == 1);
    add(QStringLiteral("pos_right"), tr("显示位置：右侧"), true, m_posPref == 2);

    QVariantMap menu;
    menu["items"] = items;
    return QString::fromUtf8(QJsonDocument::fromVariant(menu).toJson(QJsonDocument::Compact));
}

void EyePlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)
    if (menuId == QLatin1String("pos_left"))
        setPosPref(0);
    else if (menuId == QLatin1String("pos_center"))
        setPosPref(1);
    else if (menuId == QLatin1String("pos_right"))
        setPosPref(2);
}

int EyePlugin::itemSortKey(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    // 值越小越靠左
    switch (m_posPref) {
    case 0: return 5;    // 左侧
    case 2: return 95;   // 右侧
    default: return 50;  // 居中
    }
}

void EyePlugin::setSortKey(const QString &itemKey, const int order)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(order)
}

bool EyePlugin::pluginIsAllowDisable()
{
    return true;
}

bool EyePlugin::pluginIsDisable()
{
    return !m_settings.value(QStringLiteral("enabled"), true).toBool();
}

void EyePlugin::pluginStateSwitched()
{
    const bool disabled = pluginIsDisable();
    m_settings.setValue(QStringLiteral("enabled"), disabled);
    if (!disabled)
        m_proxyInter->itemAdded(this, pluginName());
    else
        m_proxyInter->itemRemoved(this, pluginName());
}

void EyePlugin::refreshIcon(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    if (m_widget)
        m_widget->update();
}

void EyePlugin::positionChanged(const Dock::Position position)
{
    m_dockPos = position;
}

// 任务栏可能被拖到屏幕四边，而 positionChanged() 只在「变化时」被调用，
// 插件加载时不一定触发。dde-dock 会把当前位置实时写到 QApplication 的
// 动态属性 PROP_POSITION 上（见 docksettings.cpp），内置插件都是这么读的，
// 这里同样优先用它。
Dock::Position EyePlugin::dockPosition() const
{
    if (QCoreApplication *app = QCoreApplication::instance()) {
        const QVariant v = app->property(PROP_POSITION);
        if (v.isValid()) {
            const Dock::Position pos = v.value<Dock::Position>();
            const int raw = int(pos);
            if (raw >= int(Dock::Top) && raw <= int(Dock::Left))
                return pos;
        }
    }
    return m_dockPos;
}

void EyePlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode)
}

// ===========================================================================
// 状态
// ===========================================================================
void EyePlugin::setEyeStyle(int s)
{
    if (s < 0 || s >= eyeThemeCount() || s == m_eyeStyle)
        return;
    m_eyeStyle = s;
    m_settings.setValue(QStringLiteral("eyeStyle"), s);
    Q_EMIT styleChanged();
}

void EyePlugin::setPetStyle(int s)
{
    if (s < -1 || s >= petCount() || s == m_petStyle)
        return;
    m_petStyle = s;
    m_settings.setValue(QStringLiteral("petStyle"), s);
    Q_EMIT styleChanged();
}

void EyePlugin::setPosPref(int p)
{
    if (p < 0 || p > 2 || p == m_posPref)
        return;
    m_posPref = p;
    m_settings.setValue(QStringLiteral("posPref"), p);
    if (m_proxyInter)
        m_proxyInter->itemUpdate(this, pluginName());
    Q_EMIT styleChanged();
}

bool EyePlugin::categoryEnabled(int idx) const
{
    if (idx < 0 || idx >= kCategoryCount)
        return false;
    const int mask = m_settings.value(QStringLiteral("enabledCategories"), 0x3FF).toInt();
    return (mask & (1 << idx)) != 0;
}

void EyePlugin::setCategoryEnabled(int idx, bool en)
{
    if (idx < 0 || idx >= kCategoryCount)
        return;
    int mask = m_settings.value(QStringLiteral("enabledCategories"), 0x3FF).toInt();
    if (en)
        mask |= (1 << idx);
    else
        mask &= ~(1 << idx);
    m_settings.setValue(QStringLiteral("enabledCategories"), mask);
}

void EyePlugin::pollCursor()
{
    const QPoint p = QCursor::pos();
    if (p != m_cursorPos) {
        m_cursorPos = p;
        if (m_widget)
            m_widget->update();
    }
}

// ===========================================================================
// 学习数据
// ===========================================================================
bool EyePlugin::loadCategories()
{
    m_catNames.clear();
    m_catNames << tr("唐诗三百首") << tr("宋词三百首") << tr("诗经")
               << tr("雅思词汇") << tr("托福词汇") << tr("大学四级词汇")
               << tr("大学六级词汇") << tr("高考核心词汇") << tr("常用英语2000词")
               << tr("维基百科精选词条");

    m_catFiles.clear();
    m_catFiles << QStringLiteral("tangshi.json") << QStringLiteral("songci.json")
               << QStringLiteral("shijing.json")
               << QStringLiteral("ielts.json") << QStringLiteral("toefl.json")
               << QStringLiteral("cet4.json") << QStringLiteral("cet6.json")
               << QStringLiteral("gaokao.json") << QStringLiteral("c2000.json")
               << QStringLiteral("wiki.json");

    m_categories.clear();
    m_shown.clear();
    for (int i = 0; i < kCategoryCount; ++i) {
        QVector<QVariantMap> items;
        QFile f(QString::fromLatin1(kDataDir) + QLatin1Char('/') + m_catFiles.at(i));
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isArray()) {
                for (const QJsonValue &v : doc.array()) {
                    if (v.isObject())
                        items.append(v.toObject().toVariantMap());
                }
            }
        }
        m_categories.append(items);
        m_shown.append(QVector<int>(items.size(), 0));
    }
    return true;
}

void EyePlugin::pickNextLearningItem()
{
    QVector<int> enabled;
    for (int i = 0; i < kCategoryCount; ++i) {
        if (categoryEnabled(i) && !m_categories.at(i).isEmpty())
            enabled.append(i);
    }
    if (enabled.isEmpty()) {
        m_learnItem.clear();
        m_learnEmpty = true;
        Q_EMIT learningChanged();
        return;
    }

    const int cat = enabled.at(QRandomGenerator::global()->bounded(enabled.size()));
    const QVector<QVariantMap> &items = m_categories.at(cat);
    QVector<int> &shown = m_shown[cat];

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
        if (r <= 0.0) {
            idx = i;
            break;
        }
    }
    shown[idx] += 1;

    QVariantMap m = items.at(idx);
    m.insert(QStringLiteral("catIndex"), cat);
    m.insert(QStringLiteral("catName"), m_catNames.at(cat));
    m.insert(QStringLiteral("type"),
             cat == 9 ? QStringLiteral("wiki")
                      : (cat <= 2 ? QStringLiteral("poem") : QStringLiteral("word")));
    m_learnItem = m;
    m_learnEmpty = false;
    Q_EMIT learningChanged();
}

QString EyePlugin::chipText() const
{
    if (m_learnEmpty)
        return tr("学习内容");
    return m_learnItem.value(QStringLiteral("catName")).toString();
}

// 组装当前学习内容的纯文本（供复制），与 V25 版 currentContentText() 一致
QString EyePlugin::currentPlainText() const
{
    if (m_learnEmpty)
        return QString();
    const QString type = m_learnItem.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("poem")) {
        QString s = m_learnItem.value(QStringLiteral("t")).toString() + QLatin1Char('\n')
                    + m_learnItem.value(QStringLiteral("a")).toString();
        const QString sec = m_learnItem.value(QStringLiteral("sec")).toString();
        if (!sec.isEmpty())
            s += QStringLiteral(" · ") + sec;
        s += QStringLiteral("\n\n") + m_learnItem.value(QStringLiteral("txt")).toString();
        const QString trans = m_learnItem.value(QStringLiteral("trans")).toString();
        if (!trans.isEmpty())
            s += QStringLiteral("\n\n【译文】\n") + trans;
        const QString note = m_learnItem.value(QStringLiteral("note")).toString();
        if (!note.isEmpty())
            s += QStringLiteral("\n\n【赏析·注释】\n") + note;
        return s;
    }

    if (type == QLatin1String("wiki")) {
        return m_learnItem.value(QStringLiteral("t")).toString() + QStringLiteral("\n\n")
               + m_learnItem.value(QStringLiteral("d")).toString()
               + QStringLiteral("\n\n（来源：维基百科）");
    }

    QString s = m_learnItem.value(QStringLiteral("w")).toString();
    const QString p = m_learnItem.value(QStringLiteral("p")).toString();
    if (!p.isEmpty())
        s += QStringLiteral("  /") + p + QLatin1Char('/');
    s += QLatin1Char('\n') + m_learnItem.value(QStringLiteral("d")).toString();
    const QVariantList es = m_learnItem.value(QStringLiteral("es")).toList();
    for (int i = 0; i < es.size(); ++i) {
        const QVariantMap e = es.at(i).toMap();
        s += QStringLiteral("\n\n例句%1: ").arg(i + 1) + e.value(QStringLiteral("e")).toString();
        const QString c = e.value(QStringLiteral("c")).toString();
        if (!c.isEmpty())
            s += QLatin1Char('\n') + c;
    }
    return s;
}

// 学习内容 HTML（供 QTextBrowser 渲染），配色与 V25 弹窗一致
QString EyePlugin::currentLearnHtml() const
{
    const QString mainC = EyeStyle::textMain().name();
    const QString bodyC = EyeStyle::textBody().name();
    const QString mutedC = EyeStyle::textMuted().name();
    const QString subC = EyeStyle::textSub().name();
    const QString accentC = EyeStyle::accent().name();
    const QString dividerC = EyeStyle::divider().name();

    auto divider = [&dividerC]() {
        return QStringLiteral("<div style='height:1px;background:%1;margin:8px 0;'></div>").arg(dividerC);
    };

    if (m_learnEmpty) {
        return QStringLiteral(
                   "<div style='color:%1;font-size:14px;text-align:center;line-height:160%;'>"
                   "还没有选择学习内容 🤔<br><br>请 右键点击眼睛 → 学习内容<br>"
                   "勾选要学习的分类后再点眼睛</div>")
            .arg(subC);
    }

    const QString type = m_learnItem.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("poem")) {
        QString h;
        h += QStringLiteral("<div style='color:%1;font-size:19px;font-weight:bold;'>%2</div>")
                 .arg(mainC, m_learnItem.value(QStringLiteral("t")).toString().toHtmlEscaped());
        QString a = m_learnItem.value(QStringLiteral("a")).toString();
        const QString sec = m_learnItem.value(QStringLiteral("sec")).toString();
        if (!sec.isEmpty())
            a += QStringLiteral(" · ") + sec;
        h += QStringLiteral("<div style='color:%1;font-size:12px;'>%2</div>").arg(subC, a.toHtmlEscaped());
        h += QStringLiteral("<div style='color:%1;font-size:15px;line-height:150%;'>%2</div>")
                 .arg(bodyC, m_learnItem.value(QStringLiteral("txt")).toString().toHtmlEscaped().replace(
                                QLatin1Char('\n'), QStringLiteral("<br>")));
        h += divider();

        const QString trans = m_learnItem.value(QStringLiteral("trans")).toString();
        if (!trans.isEmpty()) {
            h += QStringLiteral("<div style='color:%1;font-size:12px;font-weight:bold;'>【译文】</div>").arg(accentC);
            h += QStringLiteral("<div style='color:%1;font-size:13px;line-height:140%;'>%2</div>")
                     .arg(bodyC, trans.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        }
        const QString note = m_learnItem.value(QStringLiteral("note")).toString();
        if (!note.isEmpty()) {
            h += QStringLiteral("<div style='color:%1;font-size:12px;font-weight:bold;'>【赏析·注释】</div>").arg(accentC);
            h += QStringLiteral("<div style='color:%1;font-size:13px;line-height:140%;'>%2</div>")
                     .arg(mutedC, note.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        }
        return h;
    }

    if (type == QLatin1String("wiki")) {
        QString h;
        h += QStringLiteral("<div style='color:%1;font-size:20px;font-weight:bold;'>📖 %2</div>")
                 .arg(mainC, m_learnItem.value(QStringLiteral("t")).toString().toHtmlEscaped());
        h += divider();
        h += QStringLiteral("<div style='color:%1;font-size:14px;line-height:160%;'>%2</div>")
                 .arg(bodyC, m_learnItem.value(QStringLiteral("d")).toString().toHtmlEscaped());
        h += QStringLiteral("<div style='color:%1;font-size:11px;'>（来源：维基百科）</div>").arg(mutedC);
        return h;
    }

    // 单词
    const QString word = m_learnItem.value(QStringLiteral("w")).toString();
    QString h;
    h += QStringLiteral("<div style='color:%1;font-size:24px;font-weight:bold;'>%2 "
                        "<a href='eye-speak:en|%3' style='text-decoration:none;'>&#128266;</a></div>")
             .arg(mainC, word.toHtmlEscaped(), speakPayload(word, QStringLiteral("en")));
    h += QStringLiteral("<div style='color:%1;font-size:13px;'>/%2/</div>")
             .arg(subC, m_learnItem.value(QStringLiteral("p")).toString().toHtmlEscaped());
    h += QStringLiteral("<div style='color:%1;font-size:14px;line-height:140%;'>%2</div>")
             .arg(bodyC, m_learnItem.value(QStringLiteral("d")).toString().toHtmlEscaped());
    h += divider();

    const QVariantList es = m_learnItem.value(QStringLiteral("es")).toList();
    for (int i = 0; i < es.size(); ++i) {
        const QVariantMap e = es.at(i).toMap();
        const QString sent = e.value(QStringLiteral("e")).toString();
        h += QStringLiteral("<div style='color:%1;font-size:12px;font-weight:bold;'>例句%2 "
                            "<a href='eye-speak:en|%3' style='text-decoration:none;'>&#128266;</a></div>")
                 .arg(accentC)
                 .arg(i + 1)
                 .arg(speakPayload(sent, QStringLiteral("en")));
        h += QStringLiteral("<div style='color:%1;font-size:14px;font-style:italic;line-height:140%;'>%2</div>")
                 .arg(bodyC, sent.toHtmlEscaped());
        h += QStringLiteral("<div style='color:%1;font-size:13px;'>%2</div>")
                 .arg(mutedC, e.value(QStringLiteral("c")).toString().toHtmlEscaped());
        h += QStringLiteral("<div style='height:6px;'></div>");
    }
    return h;
}

QString EyePlugin::aboutHtml() const
{
    const QString mainC = EyeStyle::textMain().name();
    const QString accentC = EyeStyle::accent().name();
    const QString bodyC = EyeStyle::textBody().name();
    const QString subC = EyeStyle::textSub().name();
    const QString dividerC = EyeStyle::divider().name();

    QString h;
    h += QStringLiteral("<div style='color:%1;font-size:20px;font-weight:bold;text-align:center;'>👀 关于</div>")
             .arg(mainC);
    h += QStringLiteral("<div style='height:1px;background:%1;margin:10px 0;'></div>").arg(dividerC);
    h += QStringLiteral("<div style='color:%1;font-size:14px;font-weight:bold;text-align:center;'>作者：chnome</div>")
             .arg(accentC);
    h += QStringLiteral("<div style='color:%1;font-size:13px;text-align:center;'>任务栏卡通眼珠插件</div>")
             .arg(bodyC);
    h += QStringLiteral("<div style='color:%1;font-size:12px;line-height:160%;text-align:center;'>"
                        "· 多主题眼睛 / 宠物模式（十二生肖·鱼·猫）<br>"
                        "· 点击弹出学习内容：诗经 / 唐诗宋词 / 英语词汇 / 维基百科<br>"
                        "· 含注释例句、网络语音朗读、一键复制<br>"
                        "· 瞳孔跟随参照 GEyes（gnome-applets）算法</div>")
             .arg(subC);
    return h;
}

// ===========================================================================
// TTS / 剪贴板
// ===========================================================================
void EyePlugin::copyText(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;
    QApplication::clipboard()->setText(text);
}

void EyePlugin::speak(const QString &text, const QString &lang)
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return;
    if (lang == QLatin1String("en")) {
        // 英文：有道优先（单词音质好），失败回退百度
        const QString url = QStringLiteral("https://dict.youdao.com/dictvoice?audio=%1&type=1")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)));
        fetchAndPlay(url, t, true);
    } else {
        const QString url = QStringLiteral("https://fanyi.baidu.com/gettts?lan=zh&text=%1&spd=5&source=web")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)));
        fetchAndPlay(url, t, false);
    }
}

void EyePlugin::fetchAndPlay(const QString &urlStr, const QString &text, bool enFallback)
{
    QNetworkRequest req{QUrl(urlStr)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, text, enFallback]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError
                        && (status == 0 || status == 200)
                        && looksLikeMp3(data);
        if (!ok && enFallback) {
            const QString url2 = QStringLiteral("https://fanyi.baidu.com/gettts?lan=en&text=%1&spd=5&source=web")
                                     .arg(QString::fromUtf8(QUrl::toPercentEncoding(text)));
            QNetworkRequest req2{QUrl(url2)};
            req2.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
            QNetworkReply *r2 = m_net.get(req2);
            connect(r2, &QNetworkReply::finished, this, [this, r2]() {
                r2->deleteLater();
                if (r2->error() == QNetworkReply::NoError)
                    playMp3(r2->readAll());
            });
            return;
        }
        if (ok)
            playMp3(data);
    });
}

bool EyePlugin::looksLikeMp3(const QByteArray &data)
{
    if (data.size() < 3)
        return false;
    if (data.startsWith("ID3"))
        return true;
    const uchar b0 = static_cast<uchar>(data.at(0));
    const uchar b1 = static_cast<uchar>(data.at(1));
    return (b0 == 0xFF) && ((b1 & 0xE0) == 0xE0);
}

void EyePlugin::playMp3(const QByteArray &data)
{
    if (data.isEmpty() || !looksLikeMp3(data))
        return;

    const QString path = QDir::temp().filePath(QStringLiteral("eye_tts_v20.mp3"));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(data);
    f.close();

    if (m_ttsProcess.state() != QProcess::NotRunning) {
        m_ttsProcess.kill();
        m_ttsProcess.waitForFinished(300);
    }

    // 依次尝试常见播放器
    QStringList players;
    players << QStringLiteral("ffplay") << QStringLiteral("mpg123") << QStringLiteral("paplay");
    for (const QString &bin : players) {
        QStringList args;
        if (bin == QLatin1String("ffplay")) {
            args << QStringLiteral("-nodisp") << QStringLiteral("-autoexit")
                 << QStringLiteral("-loglevel") << QStringLiteral("quiet") << path;
        } else if (bin == QLatin1String("mpg123")) {
            args << QStringLiteral("-q") << path;
        } else {
            args << path;
        }
        m_ttsProcess.start(bin, args);
        if (m_ttsProcess.waitForStarted(500))
            return;
    }
}

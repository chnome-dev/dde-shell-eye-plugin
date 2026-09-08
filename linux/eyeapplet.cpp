// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
#include "eyeapplet.h"

#include <pluginfactory.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrl>
#include <QCursor>
#include <QClipboard>
#include <QGuiApplication>

namespace {
constexpr int kCategoryCount = 10;
}

EyeApplet::EyeApplet(QObject *parent)
    : DAppletDock(parent)
    , m_settings(QStringLiteral("deepin"), QStringLiteral("dde-shell-eye-plugin"))
{
    m_timer.setInterval(33);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        const QPoint p = QCursor::pos();
        if (p != m_cursorPos) {
            m_cursorPos = p;
            Q_EMIT cursorPosChanged();
        }
    });
}

EyeApplet::~EyeApplet() = default;

bool EyeApplet::load()
{
    return DAppletDock::load();
}

bool EyeApplet::init()
{
    DAppletDock::init();
    m_eyeStyle = m_settings.value(QStringLiteral("eyeStyle"), 0).toInt();
    m_petStyle = m_settings.value(QStringLiteral("petStyle"), -1).toInt();
    m_posPref = m_settings.value(QStringLiteral("posPref"), 0).toInt();
    setSupported(true);
    loadCategories();
    m_timer.start();
    return true;
}

int EyeApplet::cursorX() const { return m_cursorPos.x(); }
int EyeApplet::cursorY() const { return m_cursorPos.y(); }

int EyeApplet::eyeStyle() const { return m_eyeStyle; }

void EyeApplet::setEyeStyle(int style)
{
    if (style == m_eyeStyle) {
        return;
    }
    m_eyeStyle = style;
    m_settings.setValue(QStringLiteral("eyeStyle"), style);
    Q_EMIT eyeStyleChanged();
}

int EyeApplet::posPref() const
{
    return m_posPref;
}

void EyeApplet::setPosPref(int p)
{
    if (p < 0 || p > 2 || p == m_posPref) {
        return;
    }
    m_posPref = p;
    m_settings.setValue(QStringLiteral("posPref"), p);
    Q_EMIT posPrefChanged();
}

int EyeApplet::petStyle() const
{
    return m_petStyle;
}

void EyeApplet::setPetStyle(int style)
{
    if (style == m_petStyle) {
        return;
    }
    m_petStyle = style;
    m_settings.setValue(QStringLiteral("petStyle"), style);
    Q_EMIT petStyleChanged();
}

DockItemInfo EyeApplet::dockItemInfo()
{
    DockItemInfo info;
    info.name = tr("Cartoon Eye");
    info.displayName = tr("Cartoon Eye");
    info.itemKey = QStringLiteral("cartoon-eye");
    info.settingKey = QStringLiteral("cartoon-eye");
    info.dccIcon = QStringLiteral("/usr/share/dde-shell/org.deepin.ds.dock.eye/icons/eye.svg");
    info.visible = visible();
    return info;
}

// ============ 学习数据加载 ============
bool EyeApplet::loadCategories()
{
    m_catNames = {
        QStringLiteral("唐诗三百首"),
        QStringLiteral("宋词三百首"),
        QStringLiteral("诗经"),
        QStringLiteral("雅思词汇"),
        QStringLiteral("托福词汇"),
        QStringLiteral("大学四级词汇"),
        QStringLiteral("大学六级词汇"),
        QStringLiteral("高考核心词汇"),
        QStringLiteral("常用英语2000词"),
        QStringLiteral("维基百科精选词条")
    };
    m_catFiles = {
        QStringLiteral("tangshi.json"), QStringLiteral("songci.json"),
        QStringLiteral("shijing.json"),
        QStringLiteral("ielts.json"),   QStringLiteral("toefl.json"),
        QStringLiteral("cet4.json"),    QStringLiteral("cet6.json"),
        QStringLiteral("gaokao.json"),  QStringLiteral("c2000.json"),
        QStringLiteral("wiki.json")
    };

    const QString dataDir = QStringLiteral("/usr/share/dde-shell/org.deepin.ds.dock.eye/data");
    m_categories.clear();
    m_shown.clear();
    for (int i = 0; i < kCategoryCount; ++i) {
        QList<QVariantMap> items;
        QFile f(dataDir + QLatin1Char('/') + m_catFiles.at(i));
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isArray()) {
                const QJsonArray arr = doc.array();
                for (const auto &v : arr) {
                    if (v.isObject()) {
                        items.append(v.toObject().toVariantMap());
                    }
                }
            }
        }
        m_categories.append(items);
        m_shown.append(QVector<int>(items.size(), 0));
    }
    return true;
}

// ============ 分类勾选状态（位掩码持久化，默认全选） ============
bool EyeApplet::categoryEnabled(int idx) const
{
    if (idx < 0 || idx >= kCategoryCount) {
        return false;
    }
    int mask = m_settings.value(QStringLiteral("enabledCategories"), 0xFF).toInt();
    return (mask & (1 << idx)) != 0;
}

void EyeApplet::setCategoryEnabled(int idx, bool en)
{
    if (idx < 0 || idx >= kCategoryCount) {
        return;
    }
    int mask = m_settings.value(QStringLiteral("enabledCategories"), 0xFF).toInt();
    if (en) {
        mask |= (1 << idx);
    } else {
        mask &= ~(1 << idx);
    }
    m_settings.setValue(QStringLiteral("enabledCategories"), mask);
}

// ============ 加权随机取一条学习内容 ============
QVariantMap EyeApplet::nextItem()
{
    // 收集已勾选且有数据的分类
    QList<int> enabled;
    for (int i = 0; i < kCategoryCount; ++i) {
        if (categoryEnabled(i) && !m_categories.at(i).isEmpty()) {
            enabled.append(i);
        }
    }
    if (enabled.isEmpty()) {
        return QVariantMap();
    }
    // 均匀选择分类
    const int cat = enabled.at(QRandomGenerator::global()->bounded(enabled.size()));
    const auto &items = m_categories.at(cat);
    const auto &shown = m_shown.at(cat);

    // 加权随机：出现次数越多权重越低（weight = 1/(1+1.2*shown)）
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
    m_shown[cat][idx] += 1;

    QVariantMap m = items.at(idx);
    m.insert(QStringLiteral("catIndex"), cat);
    m.insert(QStringLiteral("catName"), m_catNames.at(cat));
    m.insert(QStringLiteral("type"), cat == 9 ? QStringLiteral("wiki") : (cat <= 2 ? QStringLiteral("poem") : QStringLiteral("word")));
    return m;
}

// ============ 复制到剪贴板 ============
void EyeApplet::copyText(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    QGuiApplication::clipboard()->setText(text);
}

// ============ 网络 TTS 朗读 ============
void EyeApplet::speak(const QString &text, const QString &lang)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    const QString t = text.trimmed();
    if (lang == QLatin1String("en")) {
        // 英文：有道优先（单词/短句音质好）；长句有道会 500，失败自动回退百度
        const QString url = QStringLiteral("https://dict.youdao.com/dictvoice?audio=%1&type=1")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)));
        fetchAndPlay(url, t, true);
    } else {
        // 中文：百度 TTS
        const QString url = QStringLiteral("https://fanyi.baidu.com/gettts?lan=zh&text=%1&spd=5&source=web")
                                .arg(QString::fromUtf8(QUrl::toPercentEncoding(t)));
        fetchAndPlay(url, t, false);
    }
}

void EyeApplet::fetchAndPlay(const QString &urlStr, const QString &text, bool enFallback)
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
            // 有道失败（常见于长句 500）→ 回退百度英文 TTS
            const QString url2 = QStringLiteral("https://fanyi.baidu.com/gettts?lan=en&text=%1&spd=5&source=web")
                                     .arg(QString::fromUtf8(QUrl::toPercentEncoding(text)));
            QNetworkRequest req2{QUrl(url2)};
            req2.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
            QNetworkReply *r2 = m_net.get(req2);
            connect(r2, &QNetworkReply::finished, this, [this, r2]() {
                r2->deleteLater();
                if (r2->error() == QNetworkReply::NoError) {
                    playMp3(r2->readAll());
                }
            });
            return;
        }
        if (ok) {
            playMp3(data);
        }
    });
}

bool EyeApplet::looksLikeMp3(const QByteArray &data) const
{
    if (data.size() < 3) {
        return false;
    }
    // ID3 头
    if (data.startsWith("ID3")) {
        return true;
    }
    // MPEG 帧同步字（0xFF Ex/Fx）
    const uchar b0 = static_cast<uchar>(data.at(0));
    const uchar b1 = static_cast<uchar>(data.at(1));
    return (b0 == 0xFF) && ((b1 & 0xE0) == 0xE0);
}

void EyeApplet::playMp3(const QByteArray &data)
{
    if (data.isEmpty() || !looksLikeMp3(data)) {
        return;
    }
    const QString tmp = QDir::temp().filePath(QStringLiteral("eye_tts.mp3"));
    QFile f(tmp);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
    }
    // 停止上一段播放
    if (m_ttsProcess.state() != QProcess::NotRunning) {
        m_ttsProcess.kill();
        m_ttsProcess.waitForFinished(300);
    }
    m_ttsProcess.start(QStringLiteral("ffplay"),
                       {QStringLiteral("-nodisp"), QStringLiteral("-autoexit"),
                        QStringLiteral("-loglevel"), QStringLiteral("quiet"), tmp});
}

D_APPLET_CLASS(EyeApplet)
#include "eyeapplet.moc"

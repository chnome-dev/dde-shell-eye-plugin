// SPDX-FileCopyrightText: 2026
// SPDX-License-Identifier: GPL-3.0-or-later
//
// dde-dock 插件加载冒烟测试：在不启动任务栏的情况下，复刻 dde-dock 5.x
// 的两道加载关卡，验证编出来的 .so 真的能被加载。
//
// 对应 dde-dock 源码 frame/util/abstractpluginscontroller.cpp:loadPlugin()：
//   1) QPluginLoader::metaData().MetaData.api 必须在白名单里
//      {1.1.1, 1.2, 1.2.1, DOCK_PLUGIN_API_VERSION}
//   2) instance() 成功，且 qobject_cast<PluginsItemInterface*> 非空（IID 匹配）
//
// 用法见 smoke-test.sh，或手动：
//   g++ -fPIC -std=c++14 -O1 -o loader-smoke loader-smoke-test.cpp \
//       -I<qt5-include> -I<qt5-include>/QtCore -I<qt5-include>/QtGui \
//       -I<qt5-include>/QtWidgets -I<dde-dock-include> \
//       -L<qt5-lib> -Wl,-rpath-link,<qt5-lib> \
//       -lQt5Core -lQt5Gui -lQt5Widgets
//   QT_QPA_PLATFORM=offscreen LD_LIBRARY_PATH=<qt5-lib> ./loader-smoke libcartoon-eye.so

#include <QApplication>
#include <QDebug>
#include <QJsonObject>
#include <QPluginLoader>
#include <QStringList>
#include <QWidget>

#include "pluginsiteminterface.h"
#include "constants.h"

// 与 dde-dock 完全一致的白名单
static const QStringList CompatiblePluginApiList {
    "1.1.1",
    "1.2",
    "1.2.1",
    DOCK_PLUGIN_API_VERSION
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc < 2) {
        qWarning() << "用法: loader-smoke <libcartoon-eye.so>";
        return 2;
    }
    const QString file = QString::fromLocal8Bit(argv[1]);
    qInfo().noquote() << "==== 加载:" << file;

    QPluginLoader pluginLoader(file);

    // ---- 1) 元数据 + api 白名单（dde-dock 第一道关）----
    const QJsonObject meta = pluginLoader.metaData().value("MetaData").toObject();
    const QString pluginApi = meta.value("api").toString();
    qInfo().noquote() << "[meta] api        =" << pluginApi;
    qInfo().noquote() << "[meta] name       =" << meta.value("name").toString();
    qInfo().noquote() << "[meta] version    =" << meta.value("version").toString();
    qInfo().noquote() << "[meta] pluginName =" << meta.value("pluginName[zh_CN]").toString()
                      << "/" << meta.value("pluginName[en_US]").toString();
    qInfo().noquote() << "[meta] website    =" << meta.value("website").toString();

    if (pluginApi.isEmpty() || !CompatiblePluginApiList.contains(pluginApi)) {
        qCritical().noquote() << "!! FAIL: api 版本不在白名单内 -> dde-dock 会拒载该插件";
        return 3;
    }
    qInfo().noquote() << "OK  : api 版本在白名单内" << CompatiblePluginApiList;

    // ---- 2) instance() + qobject_cast（dde-dock 第二道关）----
    QObject *obj = pluginLoader.instance();
    if (!obj) {
        qCritical().noquote() << "!! FAIL: instance() 失败:" << pluginLoader.errorString();
        return 4;
    }
    qInfo().noquote() << "OK  : instance() 成功 ->" << obj->metaObject()->className();

    PluginsItemInterface *iface = qobject_cast<PluginsItemInterface *>(obj);
    if (!iface) {
        qCritical().noquote() << "!! FAIL: qobject_cast<PluginsItemInterface*> 为空（IID 不匹配）";
        return 5;
    }
    qInfo().noquote() << "OK  : qobject_cast<PluginsItemInterface*> 成功";

    // ---- 3) 调几个不依赖 proxyInter 的虚函数，确认虚表正常 ----
    qInfo().noquote() << "[iface] pluginName()        =" << iface->pluginName();
    qInfo().noquote() << "[iface] pluginDisplayName() =" << iface->pluginDisplayName();
    qInfo().noquote() << "[iface] type()              =" << int(iface->type());

    QWidget *w = iface->itemWidget(QStringLiteral("eye"));
    qInfo().noquote() << "[iface] itemWidget()        =" << (w ? "non-null" : "null");
    if (w) {
        const QSize sh = w->sizeHint();
        qInfo().noquote() << "        sizeHint = " << sh;
        if (sh.width() < 16 || sh.width() > 100 || sh.width() != sh.height()) {
            qCritical().noquote() << "!! FAIL: sizeHint 不是 16~100 的正方形，"
                                     "在任务栏上会显示异常";
            return 6;
        }
        delete w;
    }
    qInfo().noquote() << "[iface] itemContextMenu()   =" << iface->itemContextMenu(QStringLiteral("eye")).left(120);

    qInfo().noquote() << "==== 全部通过";
    return 0;
}

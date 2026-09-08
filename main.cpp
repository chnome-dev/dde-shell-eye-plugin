// SPDX-License-Identifier: GPL-3.0-or-later
#include "eyewindow.h"
#include "learncore.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CartoonEye"));
    app.setOrganizationName(QStringLiteral("CHNOME"));
    app.setQuitOnLastWindowClosed(false);

    // 数据目录：优先使用 exe 同目录下的 data/（便于绿色部署），其次使用当前目录
    QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    if (!QDir(dataDir).exists())
        dataDir = QStringLiteral("data");

    LearnCore core;
    if (!core.loadData(dataDir)) {
        QMessageBox::critical(nullptr, QStringLiteral("卡通眼珠"),
            QStringLiteral("找不到学习数据目录 data/，请确认与程序在同一目录下。"));
        return 1;
    }

    EyeWindow win(&core);
    win.showAtPreferred();

    return app.exec();
}

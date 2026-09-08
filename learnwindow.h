// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class LearnCore;
class QLabel;
class QTextBrowser;
class QPushButton;

/** 学习内容弹窗：显示诗/词/词汇/维基词条，支持复制、换一个、朗读 */
class LearnWindow : public QDialog
{
    Q_OBJECT
public:
    explicit LearnWindow(LearnCore *core, QWidget *parent = nullptr);

    void showNext();        // 取一条新内容并显示
    void repositionNear(const QPoint &globalPos);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    void refresh(const QVariantMap &item);
    void applyTheme();

    LearnCore *m_core;
    QLabel *m_catLabel;
    QTextBrowser *m_browser;
    QPushButton *m_copyBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;
    QPushButton *m_speakerBtn = nullptr;  // 朗读当前内容
    QString m_speakText, m_speakLang;
    QPoint m_dragOffset;
    bool m_dragging = false;
    QVariantMap m_item;
};

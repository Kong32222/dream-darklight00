#ifndef SOUNDBAR_H
#define SOUNDBAR_H


#include <QWidget>
#include <QPainter>
#include <QColor>

// QT  += multimedia
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QByteArray>
#include <iostream>
#include <QAudioInput>

#include <QDebug>
#include <QtWidgets>

// 定义调试宏 用于DEBUG 打印
#ifdef ENABLE_DEBUG
#define DEBUG_COUT(x) qDebug() << x
#else
#define DEBUG_COUT(x)
#endif

#include <QMouseEvent>
#include <QCursor>

class Soundbar : public QWidget
{
    Q_OBJECT
public:
    explicit Soundbar(QWidget *parent = nullptr);
    void setVolume(int volume);



    void moveSoundbar(int x, int y); //  移动函数
    //垂直放置
    //pSoundbar->moveSoundbar(990,47);

    //水平放置
    //  pSoundbar->moveSoundbar(700,95);
protected:
    void paintEvent(QPaintEvent *event) override;

    //
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_volume;

    bool m_dragging;
    QPoint m_dragStartPos;
};

class MyToolButton : public QToolButton
{
    Q_OBJECT

public:
    MyToolButton(QWidget *parent = nullptr)
        : QToolButton(parent)
    {
        setText("信息源");
        setPopupMode(QToolButton::InstantPopup);

        // 创建主菜单
        QMenu *mainMenu = new QMenu(this);

        // 创建子菜单 1
        QMenu *submenu1 = new QMenu("源 1", this);
        mainMenu->addMenu(submenu1);

        QAction *subaction11 = new QAction("业务 1", this);
        QAction *subaction12 = new QAction("业务 2", this);
        QAction *subaction13 = new QAction("业务 3", this);
        QAction *subaction14 = new QAction("业务 4", this);
        submenu1->addAction(subaction11);
        submenu1->addAction(subaction12);
        submenu1->addAction(subaction13);
        submenu1->addAction(subaction14);

        // 创建子菜单 2
        QMenu *submenu2 = new QMenu("源 2", this);
        mainMenu->addMenu(submenu2);

        QAction *subaction21 = new QAction("业务 1", this);
        QAction *subaction22 = new QAction("业务 2", this);
        QAction *subaction23 = new QAction("业务 3", this);
        QAction *subaction24 = new QAction("业务 4", this);
        submenu2->addAction(subaction21);
        submenu2->addAction(subaction22);
        submenu2->addAction(subaction23);
        submenu2->addAction(subaction24);

        // 将主菜单设置到工具按钮
        setMenu(mainMenu);

        // 连接菜单项的触发信号到槽函数
        connect(subaction11, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction12, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction13, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction14, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction21, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction22, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction23, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
        connect(subaction24, &QAction::triggered, this, &MyToolButton::onSubactionTriggered);
    }

private slots:
    void onSubactionTriggered() {
        QAction *action = qobject_cast<QAction *>(sender());
        if (action) {
            QMessageBox::information(this, "选项触发", action->text() + " 被触发");
        }
    }
};

#endif // SOUNDBAR_H

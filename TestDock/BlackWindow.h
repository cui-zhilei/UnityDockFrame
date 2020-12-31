#ifndef BLACK_WINDOW_H
#define BLACK_WINDOW_H

#include "DockableWindow.h"

class QMenu;
class BlackWindow : public dock::DockableWindow
{
    Q_OBJECT

public:
    BlackWindow(QWidget *parent = Q_NULLPTR);
    ~BlackWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
};

//静态注册方式,无需在其他调用REGISTER_WINDOW注册
STATIC_REGISTER_WINDOW(BlackWindow, "Black Window", true)

//只声明窗口工厂，还需要在创建此窗口前调用REGISTER_WINDOW动态注册
//DEC_WINDOW_FACTORY(BlackWindow, "Black Window", true)

#endif

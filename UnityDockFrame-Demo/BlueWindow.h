#ifndef BLUEWINDOW_H
#define BLUEWINDOW_H

#include "DockableWindow.h"

class BlueWindow : public dock::DockableWindow
{
    Q_OBJECT

public:
    BlueWindow(QWidget *parent = Q_NULLPTR);
    ~BlueWindow();

protected:
    void paintEvent(QPaintEvent *event) override;

};

STATIC_REGISTER_WINDOW(BlueWindow, "Blue Window", false)

#endif

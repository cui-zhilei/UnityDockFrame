#ifndef PACKETLISTWINDOW_H
#define PACKETLISTWINDOW_H

#include "DockableWindow.h"

class GreenWindow : public dock::DockableWindow
{
    Q_OBJECT

public:
    GreenWindow(QWidget *parent = Q_NULLPTR);
    ~GreenWindow();
    
protected:
    void paintEvent(QPaintEvent *event) override;

private:
};

STATIC_REGISTER_WINDOW(GreenWindow, "Green Window", false)

#endif

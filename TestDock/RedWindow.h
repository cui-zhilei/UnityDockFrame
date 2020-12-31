#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "DockableWindow.h"

class RedWindow : public dock::DockableWindow
{
    Q_OBJECT

public:
    RedWindow(QWidget *parent = Q_NULLPTR);
    ~RedWindow();
    virtual void onContextMenu(QMenu* menu) override { (void)menu; }
    
protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
};

STATIC_REGISTER_WINDOW(RedWindow, "Red Window", false)

#endif

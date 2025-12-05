#ifndef CYANWINDOW_H
#define CYANWINDOW_H

#include "DockableWindow.h"

class CyanWindow : public dock::DockableWindow
{
    Q_OBJECT

public:
    CyanWindow(QWidget *parent = Q_NULLPTR);
    ~CyanWindow();
    
protected:
    void paintEvent(QPaintEvent *event) override;
};

STATIC_REGISTER_WINDOW(CyanWindow, "Cyan Window", false)

#endif

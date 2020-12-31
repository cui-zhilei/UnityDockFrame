#include <QPainter>

#include "CyanWindow.h"

CyanWindow::CyanWindow(QWidget *parent)
    : dock::DockableWindow(parent)
{
}

CyanWindow::~CyanWindow()
{
}

void CyanWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), Qt::cyan);
}

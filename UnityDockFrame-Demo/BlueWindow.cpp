#include <QPainter>

#include "BlueWindow.h"

BlueWindow::BlueWindow(QWidget *parent)
    : dock::DockableWindow(parent)
{
}

BlueWindow::~BlueWindow()
{
}

void BlueWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), Qt::blue);
}

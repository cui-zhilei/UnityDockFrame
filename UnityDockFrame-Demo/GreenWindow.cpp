#include <QPainter>
#include "GreenWindow.h"

GreenWindow::GreenWindow(QWidget *parent) : dock::DockableWindow(parent)
{
}

GreenWindow::~GreenWindow()
{
}

void GreenWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), Qt::green);
}

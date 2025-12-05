#include <QVBoxLayout>
#include <QPainter>

#include "BlackWindow.h"

BlackWindow::BlackWindow(QWidget *parent)
    : dock::DockableWindow(parent)
{
}

BlackWindow::~BlackWindow()
{
}

void BlackWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), Qt::black);
}

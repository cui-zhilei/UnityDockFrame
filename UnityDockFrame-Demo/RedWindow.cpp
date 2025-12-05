#include <QVBoxLayout>
#include <QPainter>
#include <QToolBar>

#include "RedWindow.h"

RedWindow::RedWindow(QWidget *parent) : dock::DockableWindow(parent)
{
    setObjectName("MessageWindow");
}

RedWindow::~RedWindow()
{
}

void RedWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), Qt::red);
}

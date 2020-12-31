#include <QDebug>
#include <QCoreApplication>

#include <TabBar.h>

namespace dock {

static const QEvent::Type ACCEPT_MOVEEVENT_FROMNOW = (QEvent::Type)QEvent::registerEventType(QEvent::User + 100);

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
    , _isAcceptMoveEvent(true)
{

    setObjectName("DockedTabBar");
    setDrawBase(false);
}

TabBar::~TabBar()
{

}

void TabBar::setAcceptMoveEvent(bool isAccept)
{
    if (isAccept)
    {
        QEvent* customEvent = new QEvent(ACCEPT_MOVEEVENT_FROMNOW);
        QCoreApplication::postEvent(this, customEvent);
    }
    else
    {
        _isAcceptMoveEvent = false;
    }
}

bool TabBar::event(QEvent* e)
{
    if (e->type() == ACCEPT_MOVEEVENT_FROMNOW)
    {
        _isAcceptMoveEvent = true;
    }
    return QTabBar::event(e);
}

void TabBar::mouseMoveEvent(QMouseEvent* e)
{
    if (!_isAcceptMoveEvent)
    {
        return;
    }
    QTabBar::mouseMoveEvent(e);
}

}

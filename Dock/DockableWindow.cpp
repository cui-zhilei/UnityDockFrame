#include "DockableWindow.h"

namespace dock {

DockableWindow::DockableWindow(QWidget *parent)
    : QWidget(parent)
{
}

DockableWindow::~DockableWindow()
{

}

QString DockableWindow::getTitle()
{
    auto pManager = dock::WindowFactoryManager::getInstance();
    if (pManager == nullptr)
    {
        return QString();
    }
    const QMetaObject *meta = metaObject();
    if (meta == nullptr)
    {
        return QString();
    }
    auto pFactory = pManager->getFactory((int)qHash(QString(meta->className())));
    if (pFactory == nullptr)
    {
        return QString();
    }
    return pFactory->getTitle();
}

int DockableWindow::windowType()
{
    const QMetaObject *meta = metaObject();
    if (meta == nullptr)
    {
        return 0;
    }
    return (int)qHash(QString(meta->className()));
}

}

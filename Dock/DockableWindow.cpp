#include <DockableWindow.h>

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
    auto pFactory = pManager->getFactory(qHash(QString(metaObject()->className())));
    return pFactory->getTitle();
}

int DockableWindow::windowType()
{
    return qHash(QString(metaObject()->className()));
}

}

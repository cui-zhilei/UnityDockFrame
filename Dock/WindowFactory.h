/**********************************************************
* @file        WindowFactory.h
* @brief    Dockable window factory class
*
* @author    Cuizhilei
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef WINDOWFACTORY_H
#define WINDOWFACTORY_H
#include "dock_global.h"
#include <QString>
class QWidget;

namespace dock {

class DockableWindow;

class DOCKSHARED_EXPORT WindowFactory
{
public:
    virtual bool isUnique() = 0;
    virtual QString getTitle() = 0;
    virtual DockableWindow* create(QWidget* p) = 0;

    virtual ~WindowFactory() {}
};

}

#endif // WINDOWFACTORY_H

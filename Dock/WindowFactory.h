/**********************************************************
* @file        WindowFactory.h
* @brief    停靠窗口工厂类
*
* @author    崔志雷
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

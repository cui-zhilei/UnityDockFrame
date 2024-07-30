/**********************************************************
* @file        WindowFactoryManager.h
* @brief    停靠窗口工厂管理类
*
* @author    崔志雷
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef WINDOWFACTORYMANAGER_H
#define WINDOWFACTORYMANAGER_H

#include <map>
#include <vector>
#include <memory>

#include "WindowFactory.h"
#include "dock_global.h"
class QWidget;

namespace dock {
class DOCKSHARED_EXPORT WindowFactoryManager
{
public:
    void registerFactory(uint typeId, WindowFactory *fac, bool needDelete = false);
    int getFactoryCount() {return (int)_factorys.size();}
    WindowFactory *getFactory(uint typeId) ;

    static WindowFactoryManager *getInstance();
    const std::map<uint, WindowFactory *> &getAllFactorys()
    {
        return _factorys;
    }

private:
    WindowFactoryManager();
    virtual ~WindowFactoryManager();
    std::map<uint, WindowFactory *> _factorys;
    std::vector<WindowFactory *> _needDeletefactorys;
};

}

#endif // WINDOWFACTORYMANAGER_H

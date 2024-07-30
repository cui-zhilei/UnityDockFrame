#include "DockableWindowPool.h"
#include "DockableWindow.h"

namespace dock {

DockableWindowPool::DockableWindowPool()
{

}

bool DockableWindowPool::isDockedWindow(QWidget *w)
{
    DockableWindow *window = qobject_cast<DockableWindow*>(w);
    return _mapVisibleWindowToTypeID.contains(window);
}

int DockableWindowPool::windowID(DockableWindow *w)
{
    auto iter = _mapVisibleWindowToTypeID.find(w);
    if (iter == _mapVisibleWindowToTypeID.end())
    {
        return -1;
    }
    return iter.value().second;
}

DockableWindow *DockableWindowPool::newWindow(uint type)
{
    auto f = WindowFactoryManager::getInstance()->getFactory(type);
    if (f == nullptr)
    {
        return nullptr;
    }
    auto iter = _mapTypeToWindowList.find(type);
    if (iter != _mapTypeToWindowList.end())
    {
        QList<DockableWindow*> windowList = iter.value();
        for (int i = 0; i < windowList.size(); i++)
        {
            DockableWindow *w = windowList[i];
            if (!_mapVisibleWindowToTypeID.contains(w))//如果窗体不可见，则加入到可见容器列表里
            {
                _mapVisibleWindowToTypeID.insert(w, qMakePair(w->windowType(), i));
                return w;
            }
            else
            {
                if (f->isUnique())
                {
                    //如果窗体是独一无二的，则返回空
                    return nullptr;
                }
            }
        }
    }
    DockableWindow *w = f->create(nullptr);
    registerWindow(w);
    return w;
}

DockableWindow *DockableWindowPool::getOneVisibleWindow(uint type)
{
    if (!_mapTypeToWindowList.contains(type))
    {
        return nullptr;
    }
    QList<DockableWindow*> list = _mapTypeToWindowList[type];
    for (int i = 0; i < list.size(); i++)
    {
        if (_mapVisibleWindowToTypeID.contains(list[i]))
        {
            return list[i];
        }
    }
    return nullptr;
}

DockableWindow *DockableWindowPool::getWindow(uint type, int winId)
{
    auto iter = _mapTypeToWindowList.find(type);
    if (iter == _mapTypeToWindowList.end())
    {
        Q_ASSERT(false);
        return nullptr;
    }
    auto windowList = iter.value();
    if (winId < 0 || winId >= windowList.size())
    {
        return newWindow(type);
    }
    DockableWindow *w = windowList[winId];
    _mapVisibleWindowToTypeID.insert(w, qMakePair(w->windowType(), winId));
    return windowList[winId];
}

int DockableWindowPool::registerWindow(DockableWindow *w)
{
    if (_mapVisibleWindowToTypeID.contains(w))
    {
        return -1;
    }

    QList<DockableWindow*> windowList;
    auto iter = _mapTypeToWindowList.find(w->windowType());
    if (iter != _mapTypeToWindowList.end())
    {
        windowList = iter.value();
    }
    windowList.append(w);

    int wId = windowList.size() - 1;
    _mapTypeToWindowList[w->windowType()] = windowList;

    _mapVisibleWindowToTypeID.insert(w, qMakePair(w->windowType(), wId));
    return wId;
}

void DockableWindowPool::deleteWindow(DockableWindow *w)
{
    if (w == nullptr)
    {
        //Q_ASSERT(false);
        return;
    }
    int wId = -1;
    auto iterWindow_TypeID = _mapVisibleWindowToTypeID.find(w);
    if (iterWindow_TypeID == _mapVisibleWindowToTypeID.end())
    {
        return;
    }
    wId = iterWindow_TypeID.value().second;
    auto type = static_cast<int>(iterWindow_TypeID.value().first);
    _mapVisibleWindowToTypeID.erase(iterWindow_TypeID);

    auto iterType_WindowList = _mapTypeToWindowList.find(type);
    if (iterType_WindowList == _mapTypeToWindowList.end())
    {
        return;
    }

    QList<DockableWindow*> windowList = iterType_WindowList.value();
    windowList.removeAt(wId);
    _mapTypeToWindowList[iterType_WindowList.key()] = windowList;
    for (int i = wId; i < windowList.size(); i++)
    {
        _mapVisibleWindowToTypeID[windowList[i]] = qMakePair(type, i);
    }
}

void DockableWindowPool::hideAllWindowsBeforeChangeLayout()
{
    for (auto iter = _mapVisibleWindowToTypeID.begin(); iter != _mapVisibleWindowToTypeID.end(); iter++)
    {
        DockableWindow *window = iter.key();
        window->setParent(nullptr);
    }
    _mapVisibleWindowToTypeID.clear();
}

bool DockableWindowPool::hasWindow(int type)
{
    if (!_mapTypeToWindowList.contains(type))
    {
        return false;
    }
    QList<DockableWindow*> list = _mapTypeToWindowList[type];
    return !list.empty();
}

bool DockableWindowPool::hasVisibleWindow(int type)
{
    if (!_mapTypeToWindowList.contains(type))
    {
        return false;
    }
    QList<DockableWindow*> list = _mapTypeToWindowList[type];
    for (int i = 0; i < list.size(); i++)
    {
        if (_mapVisibleWindowToTypeID.contains(list[i]))
        {
            return true;
        }
    }
    return false;
}

}

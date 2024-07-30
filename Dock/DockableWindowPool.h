/**********************************************************
* @file     DockableWindowPool.h
* @brief    窗口池,切换布局时，隐藏所有窗口,而不是关闭所有窗口；
* 			目的是，保证处于已有窗口内容不变
* @author   崔志雷
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef DOCKABLEWINDOWMANAGER_H
#define DOCKABLEWINDOWMANAGER_H

#include <QSet>
#include <QMap>

#include "WindowFactoryManager.h"

class QWidget;

namespace dock {
class DockableWindow;

class DockableWindowPool
{
public:
    DockableWindowPool();

    bool isDockedWindow(QWidget *w);
    int windowID(DockableWindow *w);
    int registerWindow(DockableWindow* w);

    DockableWindow *newWindow(uint type = 0);
    DockableWindow *getWindow(uint type, int winId);
    DockableWindow *getOneVisibleWindow(uint type);

    void deleteWindow(DockableWindow* w);
    void hideAllWindowsBeforeChangeLayout();

    bool hasWindow(int type);
    bool hasVisibleWindow(int type);

private:
    //存储目前处于显示状态的窗口
    QMap<DockableWindow *, QPair<uint /*type*/, int /*id*/>> _mapVisibleWindowToTypeID;

    //存储所有窗口
    QMap<uint, QList<DockableWindow *>> _mapTypeToWindowList;
};
}

#endif // DOCKABLEWINDOWMANAGER_H

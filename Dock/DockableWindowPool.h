/**********************************************************
* @file     DockableWindowPool.h
* @brief    Window pool, when switching layouts, hide all windows instead of closing them;
*           The purpose is to ensure that the content of existing windows remains unchanged
* @author   Cuizhilei
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
    DockableWindow *getOneExistedWindow(uint type);
    DockableWindow* getFistVisibleWindow(uint type);

    void deleteWindow(DockableWindow* w);
    void hideAllWindowsBeforeChangeLayout();

    bool hasWindow(int type);
    bool hasVisibleWindow(int type);

private:
    // Store currently visible windows
    QMap<DockableWindow *, QPair<uint /*type*/, int /*id*/>> _mapVisibleWindowToTypeID;

    // Store all windows
    QMap<uint, QList<DockableWindow *>> _mapTypeToWindowList;
};
}

#endif // DOCKABLEWINDOWMANAGER_H

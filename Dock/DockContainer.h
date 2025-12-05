/**********************************************************
* @file        DockContainer.h
* @brief    Custom dockable window container class
*
* @author    Cuizhilei
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef DOCKCONTAINER_H
#define DOCKCONTAINER_H

#include "dock_global.h"
#include <memory>

#include <QObject>
#include <QSet>
#include <QRect>
#include <QPoint>

class QTabWidget;
class QMenu;

namespace dock {
class TabWidget;
class TabBar;
class LayoutManager;
class DockableWindow;
class DockableWindowPool;
class Splitter;

class DOCKSHARED_EXPORT DockContainer : public QObject
{
    Q_OBJECT
public:
    explicit DockContainer(QWidget *parent = nullptr);
    virtual ~DockContainer();

    void tabbedView(DockableWindow *view, TabWidget *tabWidget, int index, const QString &label);
    TabWidget *floatView(DockableWindow *view, const QString &title);
    void floatView(uint nWindowType);
    void activeView(uint nWindowType);

    void saveLayoutToJson(QJsonObject &jsonObj);
    void createLayoutFromJson(const QJsonObject &jsonObj);
    void enableDrag(bool bEnable);

    virtual void initLayout();
    DockableWindow* getFirstVisibleWindow(uint type);

private slots:
    void onSplitterDestroyed(QObject *obj);
    void onTabBarDestroyed(QObject *obj);
    void onTabBarCloseTab();
    void onAddTab(int windowType);
    void onDockableWindowDestroyed(QObject *obj);
    void onTabMaxmized();

signals:
    void newLayoutAdded();

protected:
    QWidget *rootWidgetAt(QPoint pt);
    QWidget *rootWidgetof(QWidget *childWidget);

    bool eventFilter(QObject *watched, QEvent *event);

protected:
    Splitter *createSplitterWidget();
    TabWidget *floatView(QWidget *view, const QString& title, QPoint curPos);
    TabWidget *createTabWidget();

    void tabBarMousePressEvent(QObject *watched, QEvent *event);
    void tabBarMouseReleaseEvent(QObject *watched, QEvent *event);
    void tabBarMouseMoveEvent(QObject *watched, QEvent *event);
    void tabBarContextMenuEvent(QObject *watched, QEvent *event);

    void showDragging(QPoint currrentCurPos);
    void beginDragging(TabBar *sourceTabBar);
    void endDragging(QPoint endPos);

    void whenDragOnTabBar(TabBar *targetTabBar, QPoint targetPos);
    void whenDragAcceptDock(QPoint cursorPos, bool isDockAtRoot);
    void whenDragIngore(QPoint cursorPos);

    void createTemplateForm();
    QSize calcFloatTemplateFormSize();
    void hideTemplateForm();

    void endDragByTabbled(QPoint pos);
    void endDragByDockedAtChild(QPoint pos);
    void endDragByDockedAtRoot(QPoint pos);
    void endDragByFloated(QPoint pos);
    void endDragByCancelled();

    void dockAtTabWidgetRight(TabWidget *newTabWidget, TabWidget *targetTabWdiget);
    void dockAtTabWidgetLeft(TabWidget *newTabWidget, TabWidget *targetTabWdiget);
    void dockAtTabWdigetTop(TabWidget *newTabWidget, TabWidget *targetTabWdiget);
    void dockAtTabWidgetBottom(TabWidget *newTabWidget, TabWidget *targetTabWdiget);
    void dockAtRootSplitterTop(TabWidget *newTabWidget, Splitter *rootSplitter);
    void dockAtRootSplitterLeft(TabWidget *newTabWidget, Splitter *rootSplitter);
    void dockAtRootSplitterRight(TabWidget *newTabWidget, Splitter *rootSplitter);
    void dockAtRootSplitterBottom(TabWidget *newTabWidget, Splitter *rootSplitter);
    QList<int> recalcRootSplitterSizesAfterAddNew(QList<int> oldSizes, int newItem, bool isToHead);
    bool isLastTabInMainWindow(TabBar* tabBar);

    Splitter *getParentSplitter(QWidget *widget);
    Splitter *getRootSplitter(QWidget *widget);
    TabWidget *getParentTabWidget(QWidget *widget);
    void changeRootSplitter(Splitter *oldRootSplitter, Splitter *newRootSplitter);

    void relocateFloatWindowGeometry(QWidget *window, QRect geometry);
    QRect getNearestRectInDesktopRect(QRect sourceRect, const QPoint &p);

    void createMaxmizeAction(QMenu *parentMenu);
    void createCloseTabAction(QMenu *parentMenu, DockableWindow *view);
    void creatAddTabMenu(QMenu *parentMenu);
    enum DragAcceptType
    {
        TAB,
        DOCK_AT_CHILD,
        DOCK_AT_ROOT,
        FLOAT
    };
    struct HoverWidgetData
    {
        DragAcceptType type;
        QWidget *horverWidget;
        HoverWidgetData()
        {
            type = FLOAT;
            horverWidget = nullptr;
        }
    };
    void getHoverWidgetData(QPoint curPos, HoverWidgetData &data);
    void onHoverWidgetChanged(QPoint curPos, HoverWidgetData &newData);

    QWidget *widgetAt(QPoint gloabPos);

    enum RegionType
    {
        LEFT,
        TOP,
        RIGHT,
        BOTTOM,
        CENTRAL
    };

    enum WidgetType
    {
        SPLITTER = 0,
        TABWIDGE,
        VIEW
    };

    RegionType getRegionType(QPoint pt, QRect rect);
    
    Splitter *createSplitterFromJson(const QJsonObject &jsonObj);
    void saveSplitterToJson(Splitter *splitter, QJsonObject &jsonObj);
    TabWidget *createTabWidgetFromJson(const QJsonObject &jsonObj);
    void saveTabWidgetToJson(TabWidget *tabWidget, Qt::Orientation orient, QJsonObject &jsonObj);

protected:
    QWidget                *_dockRootWidget;
    QList<Splitter *>       _rootSplitterList;
    QSet<QWidget *>         _tabBarSet;

    QPoint                  _mousePressPos;
    TabWidget        	   *_sourceTabWidget;
    int                     _sourceTabIndex;
    QWidget                *_sourceView;
    QString                 _sourceTabText;

    QWidget                *_parentWidget;
    HoverWidgetData         _hoverWidgetData;
    bool                    _filterSwitch;
    bool                    _isDragging;
    QTabWidget             *_templateFormOnDrag;

    TabWidget              *_contextMenuTabWidget;
    int                     _contextMenuTabIndex;

    DockableWindow         *_maxmizedWindow;
    TabWidget              *_maxmizedWindowSourceTabWidget;
    int                     _maxmizedWindowSourceTabIndex;
    TabWidget              *_maxmizedTempTabWidget;
    bool                    _isDisConnectAll;

    DockableWindowPool     *_dockableWindowPool;
    bool                    _isDraggingCancelled;
};
}

#endif

#include <QWidget>
#include <QEvent>
#include <QApplication>
#include <QLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QTabWidget>
#include <QDesktopWidget>
#include <QMargins>
#include <QMenu>
#include <QSignalMapper>
#include <QJsonArray>

#include <DockContainer.h>
#include <TabBar.h>
#include <TabWidget.h>
#include <DockableWindow.h>
#include <DockableWindowPool.h>
#include <Splitter.h>
#include "WindowFactoryManager.h"
#include "WindowFactory.h"

namespace dock {

const int WIDGET_MIN_SIZE = 100;
const int ROOT_DOCKED_SIZE_HINT = 200;
const int TEMPLATE_FORM_OPTIMUM_SIZE = 300;

const QString c_strWidgetType           = "WidgetType";
const QString c_strOrientation          = "Orientation";
const QString c_strSize                 = "Size";
const QString c_strSplitter             = "Splitter";
const QString c_strTabWidget            = "TabWidget";
const QString c_strFloatWindowChildren  = "FloatWindowChildren";
const QString c_strSplitterChildren     = "SplitterChildren";
const QString c_strTabWidgetChildren    = "TabWidgetChildren";
const QString c_strWindowType           = "WindowType";
const QString c_strWindowID             = "WindowID";
const QString c_strWindowName           = "WindowName";
const QString c_strMainWindow           = "MainWindow";
const QString c_strCurrentLayoutName    = "CurrrentLayoutName";
const QString c_strLayoutArray          = "LayoutArray";
const QString c_strGeometry             = "Geometry";
const QString c_strLeft                 = "Left";
const QString c_strTop                  = "Top";
const QString c_strWidth                = "Width";
const QString c_strHeight               = "Height";
const QString c_strCurrentTabIndex      = "CurrentTabIndex";

DockContainer::DockContainer(QWidget *parent)
    : QObject(parent)
    , _parentWidget(parent)
    , _dockRootWidget(nullptr)
    , _sourceTabWidget(nullptr)
    , _sourceTabIndex(-1)
    , _sourceView(nullptr)
    , _filterSwitch(true)
    , _isDragging(false)
    , _templateFormOnDrag(nullptr)
    , _contextMenuTabWidget(nullptr)
    , _contextMenuTabIndex(-1)
    , _maxmizedWindow(nullptr)
    , _maxmizedWindowSourceTabWidget(nullptr)
    , _maxmizedWindowSourceTabIndex(-1)
    , _maxmizedTempTabWidget(nullptr)
    , _isDisConnectAll(false)
    , _dockableWindowPool(nullptr)
    , _isDraggingCancelled(false)
{
    setObjectName("DockContainer");

    QHBoxLayout* laytout = new QHBoxLayout(_parentWidget);
    laytout->setSpacing(0);
    laytout->setContentsMargins(0, 0, 0, 0);
    _dockableWindowPool = new DockableWindowPool();
    createDefaultLayout();
}

DockContainer::~DockContainer()
{
    _isDisConnectAll = true;
    if (nullptr != _dockRootWidget)
    {
        delete _dockRootWidget;
    }
    if (nullptr != _templateFormOnDrag)
    {
        delete _templateFormOnDrag;
    }
    if (nullptr != _dockableWindowPool)
    {
        delete _dockableWindowPool;
    }
}

void DockContainer::saveLayoutToJson(QJsonObject &jsonObj)
{
    if (_maxmizedWindow != nullptr)
    {
        onTabMaxmized();
    }
    QJsonArray floatWindowChildren;
    for (int i = 0; i < _rootSplitterList.size(); i++)
    {
        QJsonObject windowObj;
        Splitter *rootSplitter = _rootSplitterList[i];
        saveSplitterToJson(rootSplitter, windowObj);

        QWidget *floatWindow = rootSplitter->parentWidget();
        if (floatWindow)
        {
            QRect geometry = floatWindow->geometry();
            QJsonObject geometryObj;
            geometryObj.insert(c_strLeft, geometry.left());
            geometryObj.insert(c_strTop, geometry.top());
            geometryObj.insert(c_strWidth, geometry.width());
            geometryObj.insert(c_strHeight, geometry.height());
            windowObj.insert(c_strGeometry, geometryObj);
        }
        if (i == 0)
        {
            windowObj.insert(c_strWindowName, c_strMainWindow);
        }
        else
        {
            windowObj.insert(c_strWindowName, QString("%1%2").arg("Window_").arg(i));
        }
        floatWindowChildren.append(windowObj);
    }
    jsonObj.insert(c_strFloatWindowChildren, floatWindowChildren);
}

void DockContainer::createLayoutFromJson(const QJsonObject &jsonObj)
{
    _tabBarSet.clear();
    _rootSplitterList.clear();
    _dockableWindowPool->hideAllWindowsBeforeChangeLayout();
    if (nullptr != _dockRootWidget)
    {
        _parentWidget->layout()->removeWidget(_dockRootWidget);
        delete _dockRootWidget;
    }
    _dockRootWidget = new QWidget();
    _dockRootWidget->setObjectName("DockRootWidget");

    QJsonArray childreList = jsonObj.value(c_strFloatWindowChildren).toArray();
    if (childreList.isEmpty())
    {
        return;
    }
    _parentWidget->layout()->addWidget(_dockRootWidget);

    QHBoxLayout* carrierlaytout = new QHBoxLayout(_dockRootWidget);
    carrierlaytout->setSpacing(0);
    carrierlaytout->setContentsMargins(0, 0, 0, 0);
    Splitter *mainRootSplitter = createSplitterFromJson(childreList[0].toObject());
    connect(mainRootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    carrierlaytout->addWidget(mainRootSplitter);
    _rootSplitterList.append(mainRootSplitter);

    for (int i = 1; i < childreList.count(); i++)
    {
        QJsonObject windowJsonObj = childreList[i].toObject();
        Splitter *floatRootSplitter = createSplitterFromJson(windowJsonObj);
        _rootSplitterList.append(floatRootSplitter);

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(floatRootSplitter);

        QWidget *floatWindow = new QWidget(_dockRootWidget);
        floatWindow->setAttribute(Qt::WA_DeleteOnClose);
        floatWindow->setLayout(layout);
        floatWindow->setWindowFlags(Qt::Window);
        floatWindow->show();

        QJsonObject geometryObj = windowJsonObj.value(c_strGeometry).toObject();
        int left = geometryObj.value(c_strLeft).toInt();
        int top = geometryObj.value(c_strTop).toInt();
        int width = geometryObj.value(c_strWidth).toInt();
        int height = geometryObj.value(c_strHeight).toInt();
        QRect rectGeometry(left, top, width, height);
        relocateFloatWindowGeometry(floatWindow, rectGeometry);
        connect(floatRootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
        connect(floatRootSplitter, &Splitter::destroyed, floatWindow, &QWidget::deleteLater);
    }
}

inline Splitter *DockContainer::createSplitterWidget()
{
    Splitter *splitter = new Splitter();
    splitter->setOpaqueResize(false);
    return splitter;
}

void DockContainer::saveSplitterToJson(Splitter *splitter, QJsonObject &jsonObj)
{
    jsonObj.insert(c_strWidgetType, SPLITTER);
    jsonObj.insert(c_strOrientation, splitter->orientation());

    int size = 0;
    if (_rootSplitterList.contains(splitter))
    {
        size = 1;
    }
    else
    {
        Qt::Orientation orientation = getParentSplitter(splitter)->orientation();
        if (orientation == Qt::Horizontal)
        {
            size = splitter->width();
        }
        else
        {
            size = splitter->height();
        }
    }
    jsonObj.insert(c_strSize, size);

    QRect rectGeometry = splitter->geometry();
    QJsonObject geometryObj;
    geometryObj.insert(c_strLeft, rectGeometry.left());
    geometryObj.insert(c_strTop, rectGeometry.top());
    geometryObj.insert(c_strWidth, rectGeometry.width());
    geometryObj.insert(c_strHeight, rectGeometry.height());
    jsonObj.insert(c_strGeometry, geometryObj);

    QJsonObject childrenList;
    for (int i = 0; i < splitter->widgetCount(); i++)
    {
        QJsonObject childObj;

        QString childName;
        QWidget *widget = splitter->widget(i);
        if (qobject_cast<Splitter *>(widget) != nullptr)
        {
            childName = c_strSplitter;
            saveSplitterToJson(qobject_cast<Splitter *>(widget), childObj);
        }
        else if (qobject_cast<TabWidget *>(widget) != nullptr)
        {
            childName = c_strTabWidget;
            saveTabWidgetToJson(qobject_cast<TabWidget *>(widget), splitter->orientation(), childObj);
        }
        childName = QString("%1_%2_%3").arg("child").arg(i).arg(childName);
        childrenList.insert(childName, childObj);
    }
    jsonObj.insert(c_strSplitterChildren, childrenList);
}

Splitter *DockContainer::createSplitterFromJson(const QJsonObject &jsonObj)
{
    Qt::Orientation orientation = (Qt::Orientation)jsonObj.value(c_strOrientation).toInt();
    Splitter *splitter = createSplitterWidget();
    splitter->setOrientation(orientation);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QList<int> sizes;
    QJsonObject childreList = jsonObj.value(c_strSplitterChildren).toObject();
    for (auto iter = childreList.begin(); iter != childreList.end(); iter++)
    {
        QJsonObject childOject = iter.value().toObject();
        int widgetType = childOject.value(c_strWidgetType).toInt();
        switch (widgetType)
        {
        case SPLITTER:
        {
            Splitter *childSplitter = createSplitterFromJson(childOject);
            splitter->addWidget(childSplitter);
            break;
        }
        case TABWIDGE:
        {
            TabWidget *tabWidget = createTabWidgetFromJson(childOject);
            splitter->addWidget(tabWidget);
            break;
        }
        default:
            Q_ASSERT(false);
            break;
        }
        int sizeInSplitter = childOject.value(c_strSize).toInt();
        sizes.append(sizeInSplitter);
    }
    splitter->updateSizes(sizes);
    return splitter;
}

void DockContainer::saveTabWidgetToJson(TabWidget *tabWidget, Qt::Orientation orient, QJsonObject &jsonObj)
{ 
    jsonObj.insert(c_strWidgetType, TABWIDGE);
    int size = 0;
    if (orient == Qt::Horizontal)
    {
        size = tabWidget->width();
    }
    else
    {
        size = tabWidget->height();
    }
    jsonObj.insert(c_strSize, size);
    jsonObj.insert(c_strCurrentTabIndex, tabWidget->tabBar()->currentIndex());

    QJsonObject children;
    for (int i = 0; i < tabWidget->widgetCount(); i++)
    {
        DockableWindow *dockableWindow = qobject_cast<DockableWindow*>(tabWidget->widget(i));
        uint wType = dockableWindow->windowType();
        QJsonObject childObj;
        childObj.insert(c_strWidgetType, VIEW);
        childObj.insert(c_strWindowType, QString::number(wType));
        //if is lastmodification todo
        {
            int wId = _dockableWindowPool->windowID(dockableWindow);
            wId = jsonObj.value(c_strWindowID).toInt();
            dockableWindow->saveObject(childObj);
        }
        QString childName = QString("%1_%2_%3").arg("Tab").arg(i).arg(wType);
        children.insert(childName, childObj);
    };
    jsonObj.insert(c_strTabWidgetChildren, children);
}

TabWidget *DockContainer::createTabWidgetFromJson(const QJsonObject &jsonObj)
{
    int currentTabIndex = jsonObj.value(c_strCurrentTabIndex).toInt();
    int wId = -1;
    if (jsonObj.contains(c_strWindowID))
    {
        wId = jsonObj.value(c_strWindowID).toInt();
    }
    TabWidget *tabWidget = createTabWidget();
    QJsonObject childreList = jsonObj.value(c_strTabWidgetChildren).toObject();
    for (auto iter = childreList.begin(); iter != childreList.end(); iter++)
    {
        QJsonObject childOject = iter.value().toObject();
        Q_ASSERT(childOject.value(c_strWidgetType).toInt() == VIEW);
        auto windowType = childOject.value(c_strWindowType).toString().toLongLong();
        DockableWindow *dockableWindow = nullptr;
        if (wId < 0)
        {
            dockableWindow = _dockableWindowPool->newWindow(windowType);
        }
        else
        {
            dockableWindow = _dockableWindowPool->getWindow(windowType, wId);
        }
        if (dockableWindow)
        {
            dockableWindow->load(childOject);
            dockableWindow->setMinimumSize(WIDGET_MIN_SIZE, WIDGET_MIN_SIZE);
            dockableWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            connect(dockableWindow, SIGNAL(destroyed(QObject *)), this, SLOT(onDockableWindowDestroyed(QObject *)));
            tabWidget->addTab(dockableWindow, dockableWindow->getTitle());
        }            }
    tabWidget->setCurrentTabIndex(currentTabIndex);
    return tabWidget;
}

void DockContainer::tabbedView(DockableWindow *view, TabWidget *tabWidget, int index, const QString &label)
{
    int actIndex = index;
    if (nullptr == tabWidget)
    {
        TabWidget *rootTab = qobject_cast<TabWidget *>(_rootSplitterList[0]->widget(0));
        actIndex = rootTab->insertTab(index, view, label);
    }
    else
    {
        actIndex = tabWidget->insertTab(index, view, label);
    }
    tabWidget->setCurrentTabIndex(actIndex);
    //save for searching
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setMinimumSize(WIDGET_MIN_SIZE, WIDGET_MIN_SIZE);
    _dockableWindowPool->registerWindow(view);
    connect(view, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
}

TabWidget *DockContainer::floatView(DockableWindow *view, const QString& title)
{
    QPoint cusPos = QApplication::desktop()->availableGeometry(view).center();
    return floatView(view, title, cusPos);
}

void DockContainer::floatView(uint nWindowType)
{
    auto pDockableWindow = _dockableWindowPool->newWindow(nWindowType);
    if (pDockableWindow != nullptr)
    {
        floatView(pDockableWindow, pDockableWindow->getTitle());
    }
}

void DockContainer::activeView(uint nWindowType)
{
    if (_dockableWindowPool->hasVisibleWindow(nWindowType))
    {
        auto pDockableWindow = _dockableWindowPool->getOneVisibleWindow(nWindowType);
        TabWidget *pParentTabWidget = getParentTabWidget(pDockableWindow);
        if (pParentTabWidget != nullptr)
        {
            pParentTabWidget->setCurrentWidget(pDockableWindow);
        }
    }
    else
    {
        floatView(nWindowType);
    }
}

TabWidget *DockContainer::floatView(QWidget *view, const QString& title, QPoint cusPos)
{
    Splitter *rootSplitter = createSplitterWidget();
    TabWidget *tabWidget = createTabWidget();
    tabWidget->addTab(view, title);
    rootSplitter->addWidget(tabWidget);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(rootSplitter);

    QWidget *floatWindow = new QWidget(_dockRootWidget);
    floatWindow->setAttribute(Qt::WA_DeleteOnClose);
    floatWindow->setLayout(layout);
    floatWindow->setWindowFlags(Qt::Window);
    DockableWindow *pDockableWindow = qobject_cast<DockableWindow *>(view);
    if (pDockableWindow != nullptr && !pDockableWindow->canClose())
    {
        floatWindow->setWindowFlags(floatWindow->windowFlags() & ~Qt::WindowCloseButtonHint);
    }
    floatWindow->show();
    QRect geo(0, 0, 900, 600);
    geo.moveCenter(cusPos);
    relocateFloatWindowGeometry(floatWindow, geo);

    //save for searching
    _dockableWindowPool->registerWindow(qobject_cast<DockableWindow*>(view));
    connect(view, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
    _rootSplitterList.append(rootSplitter);
    connect(rootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    connect(rootSplitter, &Splitter::destroyed, floatWindow, &QWidget::deleteLater);
    return tabWidget;
}

void DockContainer::relocateFloatWindowGeometry(QWidget *floatWindow, QRect geometry)
{
    QRect rect = geometry;
    if (_sourceTabWidget != nullptr)
    {
        rect = _sourceTabWidget->rect();
        rect.moveCenter(geometry.center());
    }
    rect = getNearestRectInDesktopRect(rect, rect.center());
    //add title bar height
    rect.moveTop(rect.top() + floatWindow->frameGeometry().height() - floatWindow->geometry().height());
    floatWindow->setGeometry(rect);
}

QRect DockContainer::getNearestRectInDesktopRect(QRect sourceRect, const QPoint& p)
{
    QRect desktopRect = QApplication::desktop()->availableGeometry(p);
    if (sourceRect.right() > desktopRect.right())
    {
        sourceRect.moveRight(desktopRect.right());
    }
    if (sourceRect.left() < desktopRect.left())
    {
        sourceRect.moveLeft(desktopRect.left());
    }
    if (sourceRect.bottom() > desktopRect.bottom())
    {
        sourceRect.moveBottom(desktopRect.bottom());
    }
    if (sourceRect.top() < desktopRect.top())
    {
        sourceRect.moveTop(desktopRect.top());
    }
    return sourceRect;
}

TabWidget *DockContainer::createTabWidget()
{
    TabWidget *tabWidget = new TabWidget();
    tabWidget->tabBar()->installEventFilter(this);
    tabWidget->setAttribute(Qt::WA_DeleteOnClose);
    //save for searching
    _tabBarSet.insert(tabWidget->tabBar());
    connect(tabWidget->tabBar(), &TabBar::destroyed, this, &DockContainer::onTabBarDestroyed);
    return tabWidget;
}

bool DockContainer::eventFilter(QObject *watched, QEvent *event)
{
    if (_filterSwitch && watched != this) //tarbars
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            {
                tabBarMousePressEvent(watched, event);
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                tabBarMouseReleaseEvent(watched, event);
                break;
            }
            case QEvent::MouseMove:
            {
                tabBarMouseMoveEvent(watched, event);
                break;
            }
            case QEvent::ContextMenu:
            {
                tabBarContextMenuEvent(watched, event);
            }
            case QEvent::KeyPress:
            {
                if (_isDragging)
                {
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                    if (keyEvent->key() == Qt::Key_Escape)
                    {
                        _isDraggingCancelled = true;
                        endDragging(QPoint(-1, -1));
                    }
                }
            }
            if (event->type() == QEvent::ContextMenu)
            {

            }
            default:
                break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void DockContainer::tabBarMousePressEvent(QObject *watched, QEvent *event)
{
    (void)watched;

    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    _mousePressPos = mouseEvent->globalPos();
}

void DockContainer::tabBarMouseReleaseEvent(QObject *watched, QEvent *event)
{
    (void)watched;
    if (_isDraggingCancelled)
    {
        _isDraggingCancelled = false;
    }
    else
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent *>(event);
        endDragging(mouseEvent->globalPos());
    }
    for (auto iter = _tabBarSet.begin(); iter != _tabBarSet.end(); iter++)
    {
        QWidget *tabBar = *iter;
        TabWidget *tabWidget = qobject_cast<TabWidget *>(tabBar->parentWidget());
        tabWidget->endDragging();
    }
    _mousePressPos.setX(-1);
    _mousePressPos.setY(-1);
}

void DockContainer::tabBarMouseMoveEvent(QObject *watched, QEvent *event)
{
    QMouseEvent* mouseEvent = static_cast<QMouseEvent *>(event);
    if ((mouseEvent->globalPos() - _mousePressPos).manhattanLength() > QApplication::startDragDistance())
    {
        TabBar *tabBar = qobject_cast<TabBar *>(watched);
        beginDragging(tabBar);
        if (_isDragging)
        {
            showDragging(mouseEvent->globalPos());
        }
    }
}

void DockContainer::tabBarContextMenuEvent(QObject *watched, QEvent *event)
{
    QContextMenuEvent *contexMenuEvent = static_cast<QContextMenuEvent*>(event);
    TabBar *tabBar = qobject_cast<TabBar *>(watched);
    _contextMenuTabIndex = tabBar->tabAt(contexMenuEvent->pos());
    _contextMenuTabWidget = getParentTabWidget(tabBar);
    if (_contextMenuTabIndex >= 0)
    {
        QMenu *menu = new QMenu();
        DockableWindow *view = qobject_cast<DockableWindow*>(_contextMenuTabWidget->widget(_contextMenuTabIndex));
        view->onContextMenu(menu);
        if (!menu->isEmpty())
        {
            menu->addSeparator();
        }
        createMaxmizeAction(menu);
        createCloseTabAction(menu, view);
        menu->addSeparator();
        creatAddTabMenu(menu);
        menu->exec(contexMenuEvent->globalPos());
        menu->deleteLater();
    }
}

void DockContainer::createMaxmizeAction(QMenu *parentMenu)
{
    QAction* maxmizeTabAction = parentMenu->addAction(QStringLiteral("最大化"));
    if (_maxmizedWindow != nullptr)
    {
        maxmizeTabAction->setCheckable(true);
        maxmizeTabAction->setChecked(true);
    }
    if (_contextMenuTabWidget != _maxmizedTempTabWidget)
    {
        if (getRootSplitter(_contextMenuTabWidget) != _rootSplitterList[0])
        {
            maxmizeTabAction->setEnabled(false);
        }
    }
    connect(maxmizeTabAction, &QAction::triggered, this, &DockContainer::onTabMaxmized);
}

void DockContainer::createCloseTabAction(QMenu *parentMenu, DockableWindow *view)
{
    QAction* closeTabAciton = parentMenu->addAction(QStringLiteral("关闭"));
    if (_maxmizedWindow != nullptr || !view->canClose())
    {
        closeTabAciton->setEnabled(false);
    }
    else if (isLastTabInMainWindow(_contextMenuTabWidget->tabBar()))
    {
        closeTabAciton->setEnabled(false);
    }
    connect(closeTabAciton, &QAction::triggered, this, &DockContainer::onTabBarCloseTab);
}

void DockContainer::creatAddTabMenu(QMenu *parentMenu)
{
    QSignalMapper *signalMapper = new QSignalMapper(parentMenu);
    QMenu *addTabMenu = parentMenu->addMenu(QStringLiteral("打开"));
    auto allFactorires = WindowFactoryManager::getInstance()->getAllFactorys();
    for (auto iter = allFactorires.begin(); iter != allFactorires.end(); iter++)
    {
        auto factory = iter->second;
        if (factory)
        {
            QAction* action = addTabMenu->addAction(factory->getTitle());
            if (factory->isUnique() && _dockableWindowPool->hasVisibleWindow(iter->first))
            {
                //如果窗口是全局唯一的，添加菜单禁灰
                action->setEnabled(false);
                continue;
            }
            connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
            signalMapper->setMapping(action, (uint)iter->first);
            if (_contextMenuTabWidget == _maxmizedTempTabWidget)
            {
                addTabMenu->setEnabled(false);
            }
        }    
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(onAddTab(int)));
}

bool DockContainer::isLastTabInMainWindow(TabBar *tabBar)
{
    if (tabBar->count() > 1)
    {
        return false;
    }
    if (!_parentWidget->isAncestorOf(tabBar))
    {
        return false;
    }
    if (getRootSplitter(tabBar) != _rootSplitterList[0])
    {
        return false;
    }

    for (auto iter = _tabBarSet.begin(); iter != _tabBarSet.end(); iter++)
    {
        if (*iter == tabBar)
        {
            continue;
        }
        if (_maxmizedTempTabWidget != nullptr)
        {
            if (*iter == _maxmizedTempTabWidget->tabBar())
            {
                continue;
            }
        }
        if (getRootSplitter(*iter) != _rootSplitterList[0])
        {
            continue;
        }
        return false;
    }
    return true;
}

void DockContainer::beginDragging(TabBar *tabBar)
{
    if (!_isDragging)
    {
        if (_maxmizedTempTabWidget != nullptr)
        {
            if (_maxmizedTempTabWidget->tabBar() == tabBar)
            {
                return;
            }
        }
        if (isLastTabInMainWindow(tabBar))
        {
            return;
        }
        for (auto iter = _tabBarSet.begin(); iter != _tabBarSet.end(); iter++)
        {
            QWidget *tabBar = *iter;
            TabWidget *tabWidget = qobject_cast<TabWidget *>(tabBar->parentWidget());
            tabWidget->beginDragging();
        }
        _sourceTabWidget = qobject_cast<TabWidget *>(tabBar->parent());
        QPoint localPoint = tabBar->mapFromGlobal(_mousePressPos);
        _sourceTabIndex = tabBar->tabAt(localPoint);
        if (_sourceTabIndex >= 0)
        {
            _sourceTabText = _sourceTabWidget->tabText(_sourceTabIndex);
            _sourceView = _sourceTabWidget->widget(_sourceTabIndex);
            _sourceTabWidget->removeOnlyTab(_sourceTabIndex);
            _isDragging = true;
            qApp->installEventFilter(this);
        }
    }
}

void DockContainer::endDragging(QPoint pos)
{
    if (_isDragging)
    {
        if (_isDraggingCancelled && _hoverWidgetData.type == TAB)
        {
            //拖拽成tab热点显示状态时，不允许取消。2020.10解决崩溃bug
            return;
        }
        qApp->removeEventFilter(this);
        if (_isDraggingCancelled && _hoverWidgetData.horverWidget != _sourceTabWidget->tabBar())
        {
            //if be cancelled
            endDragByCancelled();
        }
        else if (_hoverWidgetData.type == FLOAT)
        {
            endDragByFloated(pos);
        }
        else
        {
            _parentWidget->setUpdatesEnabled(false);
            if (_hoverWidgetData.type == TAB)
            {
                endDragByTabbled(pos);
            }
            else if (_hoverWidgetData.type == DOCK_AT_ROOT)
            {
                endDragByDockedAtRoot(pos);
            }
            else //if dock
            {
                endDragByDockedAtChild(pos);
            }
        }
        if (_sourceTabWidget->widgetCount() == 0)
        {
            _sourceTabWidget->deleteLater();
        }
        _hoverWidgetData.type = FLOAT;
        _hoverWidgetData.horverWidget = nullptr;
        _isDragging = false;
        _sourceTabIndex = -1;
        _sourceTabText = "";
        _sourceView = nullptr;
        _sourceTabWidget = nullptr;
        hideTemplateForm();
        QApplication::processEvents();
        _parentWidget->setUpdatesEnabled(true);
        qApp->removeEventFilter(this);
    }
}

void DockContainer::endDragByTabbled(QPoint pos)
{
    TabBar *targetTabBar = qobject_cast<TabBar *>(_hoverWidgetData.horverWidget);
    TabWidget *targetTabWidget = qobject_cast<TabWidget *>(targetTabBar->parent());
    QPoint localPos = targetTabWidget->tabBar()->mapFromGlobal(pos);
    int index = targetTabWidget->tabBar()->tabAt(localPos);
    targetTabWidget->removeTempTab();
    _sourceTabWidget->removeOnlyWidget(_sourceView);
    int newIndex = targetTabWidget->insertTab(index, _sourceView, _sourceTabText);
    targetTabWidget->setCurrentTabIndex(newIndex);
    targetTabWidget->setCurrentWidgetIndex(newIndex);
}

void DockContainer::endDragByDockedAtRoot(QPoint pos)
{
    TabWidget *newTabWidget = createTabWidget();
    _sourceTabWidget->removeOnlyWidget(_sourceView);
    newTabWidget->addTab(_sourceView, _sourceTabText);
    Splitter *splitter = _hoverWidgetData.horverWidget->findChild<Splitter *>();
    if (splitter == nullptr)
    {
        Q_ASSERT(false);
        return;
    }

    QPoint locatPoint = _hoverWidgetData.horverWidget->mapFromGlobal(pos);
    RegionType type = getRegionType(locatPoint, _hoverWidgetData.horverWidget->rect());
    switch (type)
    {
    case TOP:
    {
        dockAtRootSplitterTop(newTabWidget, splitter);
        break;
    }
    case LEFT:
    {
        dockAtRootSplitterLeft(newTabWidget, splitter);
        break;
    }
    case RIGHT:
    {
        dockAtRootSplitterRight(newTabWidget, splitter);
        break;
    }
    case BOTTOM:
    {
        dockAtRootSplitterBottom(newTabWidget, splitter);
        break;
    }
    default:
        break;
    }
}

void DockContainer::endDragByDockedAtChild(QPoint pos)
{
    TabWidget *hoverTabWidget = getParentTabWidget(_hoverWidgetData.horverWidget);
    TabWidget *newTabWidget = createTabWidget();
    _sourceTabWidget->removeOnlyWidget(_sourceView);
    newTabWidget->addTab(_sourceView, _sourceTabText);
    QPoint locatPoint = hoverTabWidget->mapFromGlobal(pos);
    RegionType type = getRegionType(locatPoint, hoverTabWidget->rect());
    switch (type)
    {
    case TOP:
    {
        dockAtTabWdigetTop(newTabWidget, hoverTabWidget);
        break;
    }
    case LEFT:
    {
        dockAtTabWidgetLeft(newTabWidget, hoverTabWidget);
        break;
    }
    case RIGHT:
    {
        dockAtTabWidgetRight(newTabWidget, hoverTabWidget);
        break;
    }
    case BOTTOM:
    {
        dockAtTabWidgetBottom(newTabWidget, hoverTabWidget);
        break;
    }
    default:
        break;
    }
}


void DockContainer::dockAtTabWidgetRight(TabWidget *newTabWidget, TabWidget *hoverTabWidget)
{
    Splitter *hoverSplitter = getParentSplitter(hoverTabWidget);
    Q_ASSERT(hoverSplitter != nullptr);
    int index = hoverSplitter->indexOf(hoverTabWidget);
    int rightWidth = hoverTabWidget->width() / 3;
    int leftWidth = hoverTabWidget->width() - rightWidth;
    if (hoverSplitter->orientation() == Qt::Horizontal)
    {
        QList<int> sizes = hoverSplitter->sizes();
        hoverSplitter->insertWidget(index + 1, newTabWidget);
        sizes[index] = leftWidth;
        sizes.insert(index + 1, rightWidth);
        hoverSplitter->updateSizes(sizes);
    }
    else
    {
        Splitter *newSplitter = createSplitterWidget();
        newSplitter->setOrientation(Qt::Horizontal);
        hoverSplitter->replaceWidget(index, newSplitter);
        newSplitter->addWidget(hoverTabWidget);
        newSplitter->addWidget(newTabWidget);
        QList<int> sizes;
        sizes.append(leftWidth);
        sizes.append(rightWidth);
        newSplitter->updateSizes(sizes);
    }
}

void DockContainer::dockAtTabWidgetLeft(TabWidget *newTabWidget, TabWidget *hoverTabWidget)
{
    Splitter *hoverSplitter = getParentSplitter(hoverTabWidget);
    Q_ASSERT(hoverSplitter != nullptr);
    int index = hoverSplitter->indexOf(hoverTabWidget);
    int leftWidth = hoverTabWidget->width() / 3;
    int rightWidth = hoverTabWidget->width() - leftWidth;
    if (hoverSplitter->orientation() == Qt::Horizontal)
    {
        QList<int> sizes = hoverSplitter->sizes();
        hoverSplitter->insertWidget(index, newTabWidget);
        sizes.insert(index, leftWidth);
        sizes[index + 1] = rightWidth;
        hoverSplitter->updateSizes(sizes);
    }
    else
    {
        Splitter *newSplitter = createSplitterWidget();
        newSplitter->setOrientation(Qt::Horizontal);
        hoverSplitter->replaceWidget(index, newSplitter);
        newSplitter->addWidget(newTabWidget);
        newSplitter->addWidget(hoverTabWidget);
        QList<int> sizes;
        sizes.append(leftWidth);
        sizes.append(rightWidth);
        newSplitter->updateSizes(sizes);
    }

}

void DockContainer::dockAtTabWdigetTop(TabWidget *newTabWidget, TabWidget *hoverTabWidget)
{
    Splitter *hoverSplitter = getParentSplitter(hoverTabWidget);
    Q_ASSERT(hoverSplitter != nullptr);
    int index = hoverSplitter->indexOf(hoverTabWidget);
    int topHeight = hoverTabWidget->height() / 3;
    int bottomHeight = hoverTabWidget->height() - topHeight;
    if (hoverSplitter->orientation() == Qt::Vertical)
    {
        QList<int> sizes = hoverSplitter->sizes();
        hoverSplitter->insertWidget(index, newTabWidget);
        sizes.insert(index, topHeight);
        sizes[index + 1] = bottomHeight;
        hoverSplitter->updateSizes(sizes);
    }
    else
    {
        Splitter *newSplitter = createSplitterWidget();
        newSplitter->setOrientation(Qt::Vertical);
        hoverSplitter->replaceWidget(index, newSplitter);
        newSplitter->addWidget(newTabWidget);
        newSplitter->addWidget(hoverTabWidget);
        QList<int> sizes;
        sizes << topHeight << bottomHeight;
        newSplitter->updateSizes(sizes);
    }
}

void DockContainer::dockAtTabWidgetBottom(TabWidget *newTabWidget, TabWidget *hoverTabWidget)
{
    Splitter *hoverSplitter = getParentSplitter(hoverTabWidget);
    Q_ASSERT(nullptr != hoverSplitter);
    int index = hoverSplitter->indexOf(hoverTabWidget);
    int bottomHeight = hoverTabWidget->height() / 3;
    int topHeight = hoverTabWidget->height() - bottomHeight;
    if (hoverSplitter->orientation() == Qt::Vertical)
    {
        QList<int> sizes = hoverSplitter->sizes();
        hoverSplitter->insertWidget(index + 1, newTabWidget);
        sizes[index] = topHeight;
        sizes.insert(index + 1, bottomHeight);
        hoverSplitter->updateSizes(sizes);
    }
    else
    {
        Splitter *newSplitter = createSplitterWidget();
        newSplitter->setOrientation(Qt::Vertical);
        hoverSplitter->replaceWidget(index, newSplitter);
        newSplitter->addWidget(hoverTabWidget);
        newSplitter->addWidget(newTabWidget);
        QList<int> sizes;
        sizes << topHeight << bottomHeight;
        newSplitter->updateSizes(sizes);
    }

}

void DockContainer::dockAtRootSplitterTop(TabWidget *newTabWidget, Splitter *rootSplitter)
{
    if (rootSplitter->orientation() == Qt::Horizontal)
    {
        if (rootSplitter->widgetCount() == 1)
        {
            rootSplitter->setOrientation(Qt::Vertical);
        }
    }
    if (rootSplitter->orientation() == Qt::Vertical)
    {
        QList<int> sizes = rootSplitter->sizes();
        rootSplitter->insertWidget(0, newTabWidget);
        QList<int> newSizes = recalcRootSplitterSizesAfterAddNew(sizes, ROOT_DOCKED_SIZE_HINT, true);
        rootSplitter->updateSizes(newSizes);
    }
    else
    {
        QWidget *window = rootSplitter->parentWidget();
        Q_ASSERT(window != nullptr);
        QList<int> newSizes;
        newSizes << ROOT_DOCKED_SIZE_HINT << rootSplitter->height() - ROOT_DOCKED_SIZE_HINT;
        Splitter *newRootSplitter = createSplitterWidget();
        newRootSplitter->setOrientation(Qt::Vertical);
        newRootSplitter->addWidget(newTabWidget);
        newRootSplitter->addWidget(rootSplitter);
        newRootSplitter->updateSizes(newSizes);
        window->layout()->addWidget(newRootSplitter);
        changeRootSplitter(rootSplitter, newRootSplitter);
    }
}

void DockContainer::dockAtRootSplitterLeft(TabWidget *newTabWidget, Splitter *rootSplitter)
{
    if (rootSplitter->orientation() == Qt::Vertical)
    {
        if (rootSplitter->widgetCount() == 1)
        {
            rootSplitter->setOrientation(Qt::Horizontal);
        }
    }
    if (rootSplitter->orientation() == Qt::Horizontal)
    {
        QList<int> sizes = rootSplitter->sizes();
        rootSplitter->insertWidget(0, newTabWidget);
        QList<int> newSizes = recalcRootSplitterSizesAfterAddNew(sizes, ROOT_DOCKED_SIZE_HINT, true);
        rootSplitter->updateSizes(newSizes);
    }
    else
    {
        QWidget *window = rootSplitter->parentWidget();
        Q_ASSERT(window != nullptr);
        QList<int> newSizes;
        newSizes << ROOT_DOCKED_SIZE_HINT << rootSplitter->height() - ROOT_DOCKED_SIZE_HINT;
        Splitter *newRootSplitter = createSplitterWidget();
        newRootSplitter->setOrientation(Qt::Horizontal);
        newRootSplitter->addWidget(newTabWidget);
        newRootSplitter->addWidget(rootSplitter);
        newRootSplitter->updateSizes(newSizes);
        window->layout()->addWidget(newRootSplitter);
        changeRootSplitter(rootSplitter, newRootSplitter);
    }
}
void DockContainer::dockAtRootSplitterBottom(TabWidget *newTabWidget, Splitter *rootSplitter)
{
    if (rootSplitter->orientation() == Qt::Horizontal)
    {
        if (rootSplitter->widgetCount() == 1)
        {
            rootSplitter->setOrientation(Qt::Vertical);
        }
    }
    if (rootSplitter->orientation() == Qt::Vertical)
    {
        QList<int> sizes = rootSplitter->sizes();
        rootSplitter->addWidget(newTabWidget);
        QList<int> newSizes = recalcRootSplitterSizesAfterAddNew(sizes, ROOT_DOCKED_SIZE_HINT, false);
        rootSplitter->updateSizes(newSizes);
    }
    else
    {
        QWidget *window = rootSplitter->parentWidget();
        Q_ASSERT(window != nullptr);
        QList<int> newSizes;
        newSizes << rootSplitter->height() - ROOT_DOCKED_SIZE_HINT << ROOT_DOCKED_SIZE_HINT;
        Splitter *newRootSplitter = createSplitterWidget();
        newRootSplitter->setOrientation(Qt::Vertical);
        newRootSplitter->addWidget(rootSplitter);
        newRootSplitter->addWidget(newTabWidget);
        newRootSplitter->updateSizes(newSizes);
        window->layout()->addWidget(newRootSplitter);
        changeRootSplitter(rootSplitter, newRootSplitter);
    }
}

void DockContainer::dockAtRootSplitterRight(TabWidget *newTabWidget, Splitter *rootSplitter)
{
    if (rootSplitter->orientation() == Qt::Vertical)
    {
        if (rootSplitter->widgetCount() == 1)
        {
            rootSplitter->setOrientation(Qt::Horizontal);
        }
    }
    if (rootSplitter->orientation() == Qt::Horizontal)
    {
        QList<int> sizes = rootSplitter->sizes();
        rootSplitter->addWidget(newTabWidget);
        QList<int> newSizes = recalcRootSplitterSizesAfterAddNew(sizes, ROOT_DOCKED_SIZE_HINT, false);
        rootSplitter->updateSizes(newSizes);
    }
    else
    {
        QWidget *window = rootSplitter->parentWidget();
        Q_ASSERT(window != nullptr);
        QList<int> newSizes;
        newSizes << rootSplitter->height() - ROOT_DOCKED_SIZE_HINT << ROOT_DOCKED_SIZE_HINT;
        Splitter *newRootSplitter = createSplitterWidget();
        newRootSplitter->setOrientation(Qt::Horizontal);
        newRootSplitter->addWidget(rootSplitter);
        newRootSplitter->addWidget(newTabWidget);
        newRootSplitter->updateSizes(newSizes);
        window->layout()->addWidget(newRootSplitter);
        changeRootSplitter(rootSplitter, newRootSplitter);
    }
}

QList<int> DockContainer::recalcRootSplitterSizesAfterAddNew(QList<int> oldSizes, int newItemSizeHint, bool isToHead)
{
    QList<int> newSizes;
    int totalDecrease = 0;
    for (int i = 0; i < oldSizes.size(); i++)
    {
        int decrease = std::min<int>(newItemSizeHint - totalDecrease, oldSizes[i] - WIDGET_MIN_SIZE);
        newSizes.append(oldSizes[i] - decrease);
        totalDecrease += decrease;
        Q_ASSERT(totalDecrease <= newItemSizeHint);
        if (totalDecrease == newItemSizeHint)
        {
            continue;
        }
    }
    int newItemSize = std::max<int>(WIDGET_MIN_SIZE, totalDecrease);
    if (isToHead)
    {
        newSizes.insert(0, newItemSize);
    }
    else
    {
        newSizes.append(newItemSize);
    }
    return newSizes;
}

void DockContainer::endDragByFloated(QPoint pos)
{
    _sourceTabWidget->removeOnlyWidget(_sourceView);
    floatView(_sourceView, _sourceTabText, pos);
}

void DockContainer::endDragByCancelled()
{
    _sourceTabWidget->addTempTab(_sourceTabIndex, _sourceTabText);
    _sourceTabWidget->setCurrentTabIndex(_sourceTabIndex);
    if (_hoverWidgetData.type == TAB)
    {
        TabBar *oldTargetBar = qobject_cast<TabBar *>(_hoverWidgetData.horverWidget);
        TabWidget *oldHoverTabWidget = qobject_cast<TabWidget *>(oldTargetBar->parent());
        oldHoverTabWidget->removeTempTab();
    }
    TabBar *sourceTabBar = _sourceTabWidget->tabBar();
    QPoint tabCenter = sourceTabBar->tabRect(_sourceTabIndex).center();
    QMouseEvent mouseEventRlease(
        QEvent::MouseButtonRelease,
        QPointF(tabCenter),
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QCoreApplication::sendEvent(sourceTabBar, &mouseEventRlease);
}

Splitter *DockContainer::getParentSplitter(QWidget *widget)
{
    QWidget *parentWidget = widget->parentWidget();
    Splitter *parentSpliter = qobject_cast<Splitter *>(parentWidget);
    while (parentSpliter == nullptr && parentWidget != nullptr)
    {
        parentWidget = parentWidget->parentWidget();
        parentSpliter = qobject_cast<Splitter *>(parentWidget);
    }
    return parentSpliter;
}

Splitter *DockContainer::getRootSplitter(QWidget *widget)
{
    Splitter *parentSplitter = qobject_cast<Splitter *>(widget);
    if (parentSplitter == nullptr)
    {
        parentSplitter = getParentSplitter(widget);
    }

    Splitter *rootParentSplitter = parentSplitter;
    while (parentSplitter != nullptr)
    {
        rootParentSplitter = parentSplitter;
        parentSplitter = getParentSplitter(parentSplitter);
    }
    Q_ASSERT(nullptr != rootParentSplitter);
    return rootParentSplitter;
}

TabWidget *DockContainer::getParentTabWidget(QWidget *widget)
{
    QWidget *parentWidget = widget->parentWidget();
    TabWidget *parentTabWidget = qobject_cast<TabWidget *>(parentWidget);
    while (parentTabWidget == nullptr && parentWidget != nullptr)
    {
        parentWidget = parentWidget->parentWidget();
        parentTabWidget = qobject_cast<TabWidget *>(parentWidget);
    }
    Q_ASSERT(nullptr != parentWidget);
    return parentTabWidget;
}

void DockContainer::showDragging(QPoint currrentCurPos)
{
    HoverWidgetData hoverData;
    getHoverWidgetData(currrentCurPos, hoverData);
    if (hoverData.horverWidget == nullptr || hoverData.horverWidget != _hoverWidgetData.horverWidget)
    {
        onHoverWidgetChanged(currrentCurPos, hoverData);
    }
    if (_hoverWidgetData.type != hoverData.type)
    {
        _hoverWidgetData.type = hoverData.type;
    }
    switch (hoverData.type)
    {
        case TAB:
        {
            TabBar *targetTabBar = qobject_cast<TabBar *>(hoverData.horverWidget);
            whenDragOnTabBar(targetTabBar, currrentCurPos);
            break;
        }
        case DOCK_AT_CHILD:
        {
            whenDragAcceptDock(currrentCurPos, false);
            break;
        }
        case DOCK_AT_ROOT:
        {
            whenDragAcceptDock(currrentCurPos, true);
            break;
        }
        default:
        {
            //如果窗口不能被关闭,也就不能拖成浮动窗口
            whenDragIngore(currrentCurPos);
            break;
        }
    }
}

void DockContainer::getHoverWidgetData(QPoint curPos, HoverWidgetData &data)
{
    if (_hoverWidgetData.type == TAB
        && _hoverWidgetData.horverWidget->rect().contains(_hoverWidgetData.horverWidget->mapFromGlobal(curPos)))
    {
        data = _hoverWidgetData;
        return;
    }

    QWidget *hoverWidget = this->widgetAt(curPos);
    if (_maxmizedWindow != nullptr)
    {
        if (_parentWidget->isAncestorOf(hoverWidget)
            || hoverWidget->isAncestorOf(_parentWidget))
        {
            data.type = FLOAT;
            return;
        }
    }
    if (hoverWidget != nullptr && hoverWidget->isWindow())
    {
        data.horverWidget = hoverWidget;
        data.type = DOCK_AT_ROOT;
        return;
    }

    if (_tabBarSet.contains(hoverWidget))
    {
        data.horverWidget = hoverWidget;
        data.type = TAB;
        return;
    }

    while (nullptr != hoverWidget)
    {
        if (_dockableWindowPool->isDockedWindow(hoverWidget))
            break;
        hoverWidget = hoverWidget->parentWidget();
    }

    if (nullptr != hoverWidget)
    {
        TabWidget *tabWiget = getParentTabWidget(hoverWidget);
        if (tabWiget == _sourceTabWidget && _sourceTabWidget->widgetCount() == 1)
        {
            if(qobject_cast<QTabBar *>(hoverWidget) != nullptr)
            {
                data.type = TAB;
                data.horverWidget = hoverWidget;
            }
            else
            {
                data.type = FLOAT;
            }
        }
        else
        {
            data.horverWidget = hoverWidget;
            data.type = DOCK_AT_CHILD;
        }
        return;
    }
    data.type = FLOAT;
}

void DockContainer::onHoverWidgetChanged(QPoint curPos, HoverWidgetData &newHoverdata)
{
    _filterSwitch = false;

    if (_hoverWidgetData.type == TAB)
    {
        TabBar *oldTargetBar = qobject_cast<TabBar *>(_hoverWidgetData.horverWidget);
        QPoint tabCenter = oldTargetBar->tabRect(oldTargetBar->currentIndex()).center();
        QMouseEvent mouseEventRlease(
            QEvent::MouseButtonRelease,
            QPointF(tabCenter),
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QCoreApplication::sendEvent(_hoverWidgetData.horverWidget, &mouseEventRlease);

        TabWidget *oldHoverTabWidget = qobject_cast<TabWidget *>(oldTargetBar->parent());
        oldHoverTabWidget->removeTempTab();
    }

    if (newHoverdata.type == TAB)
    {
        TabBar *targetBar = qobject_cast<TabBar *>(newHoverdata.horverWidget);
        int tempTabIndex = targetBar->tabAt(targetBar->mapFromGlobal(curPos));
        TabWidget *targetTabWidget = qobject_cast<TabWidget *>(targetBar->parent());
        targetTabWidget->addTempTab(tempTabIndex, _sourceTabText);
        targetBar->setAcceptMoveEvent(true);

        QPoint tabCenter;
        if (tempTabIndex < 0)
        {
            tempTabIndex = targetBar->count() - 1;
        }
        tabCenter = targetBar->tabRect(tempTabIndex).center();
        QMouseEvent mouseEventpress(
            QEvent::MouseButtonPress,
            QPointF(tabCenter),
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QCoreApplication::sendEvent(newHoverdata.horverWidget, &mouseEventpress);
    }
    if (_hoverWidgetData.horverWidget == nullptr)
    {
        //when first time come here
        TabBar *oldTargetBar = qobject_cast<TabBar *>(_sourceTabWidget->tabBar());
        {
            oldTargetBar->setAcceptMoveEvent(false);
        }
    }
    _filterSwitch = true;
    _hoverWidgetData = newHoverdata;
}

void DockContainer::whenDragOnTabBar(TabBar *targetTabBar, QPoint   targetPos)
{
    hideTemplateForm();
    if (_sourceTabWidget->tabBar() == targetTabBar)
    {
        return;
    }
    _filterSwitch = false;
    QMouseEvent mouseEventMove(
        QEvent::MouseMove,
        QPointF(targetTabBar->mapFromGlobal(targetPos)),
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QCoreApplication::sendEvent(targetTabBar, &mouseEventMove);
    _filterSwitch = true;
}

void DockContainer::whenDragIngore(QPoint cursorPos)
{
    createTemplateForm();
    _templateFormOnDrag->resize(calcFloatTemplateFormSize());
    _templateFormOnDrag->setWindowOpacity(0.8);
    QRect rect = _templateFormOnDrag->rect();
    rect.moveCenter(cursorPos);
    rect = getNearestRectInDesktopRect(rect, cursorPos);

    _templateFormOnDrag->move(rect.topLeft());
    _templateFormOnDrag->raise();
    _templateFormOnDrag->show();
}

void DockContainer::whenDragAcceptDock(QPoint cursorPos, bool isDockAtRoot)
{
    createTemplateForm();
    QWidget *hoverTabWidget = _hoverWidgetData.horverWidget;
    if (!isDockAtRoot)
    {
        hoverTabWidget = getParentTabWidget(_hoverWidgetData.horverWidget);
    }
    else if(hoverTabWidget->isAncestorOf(_rootSplitterList[0]))
    {
        hoverTabWidget = _rootSplitterList[0];
    }
    QPoint locatPoint = hoverTabWidget->mapFromGlobal(cursorPos);
    RegionType type = getRegionType(locatPoint, hoverTabWidget->rect());
    if (type == CENTRAL)
    {
        _hoverWidgetData.type = FLOAT;
        whenDragIngore(cursorPos);
    }
    else
    {
        QRect rect = hoverTabWidget->rect();
        rect.moveTo(hoverTabWidget->mapToGlobal(QPoint(0, 0)));
        switch (type)
        {
        case TOP:
            rect.setBottom(rect.top() + rect.height() / 3);
            if (isDockAtRoot)
            {
                if (rect.height() > ROOT_DOCKED_SIZE_HINT)
                {
                    rect.setHeight(ROOT_DOCKED_SIZE_HINT);
                }
            }
            break;
        case LEFT:
            rect.setRight(rect.left() + rect.width() / 3);
            if (isDockAtRoot)
            {
                if (rect.width() > ROOT_DOCKED_SIZE_HINT)
                {
                    rect.setWidth(ROOT_DOCKED_SIZE_HINT);
                }
            }
            break;
        case RIGHT:
            rect.setLeft(rect.left() + rect.width() * 2 / 3);
            if (isDockAtRoot)
            {
                if (rect.width() > ROOT_DOCKED_SIZE_HINT)
                {
                    rect.setLeft(rect.right() - ROOT_DOCKED_SIZE_HINT);
                }
            }
            break;
        case BOTTOM:
            rect.setTop(rect.top() + rect.height() * 2 / 3);
            if (isDockAtRoot)
            {
                if (rect.height() > ROOT_DOCKED_SIZE_HINT)
                {
                    rect.setTop(rect.bottom() - ROOT_DOCKED_SIZE_HINT);
                }
            }
            break;
        default:
            break;
        }
        _templateFormOnDrag->setGeometry(rect);
        _templateFormOnDrag->setWindowOpacity(1.0);
        _templateFormOnDrag->raise();
        _templateFormOnDrag->show();
    }
}

void DockContainer::createTemplateForm()
{
    if (_templateFormOnDrag == nullptr)
    {
        _templateFormOnDrag = new QTabWidget(nullptr);
        _templateFormOnDrag->addTab(new QWidget(), _sourceTabText);
        _templateFormOnDrag->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
        _templateFormOnDrag->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    _templateFormOnDrag->setTabText(0, _sourceTabText);
}

QSize DockContainer::calcFloatTemplateFormSize()
{
    int with = _sourceTabWidget->width();
    int height = _sourceTabWidget->height();
    if (with < WIDGET_MIN_SIZE || height < WIDGET_MIN_SIZE)
    {
        return QSize(with, height);
    }
    float ratio = 1.0f;
    if (with > TEMPLATE_FORM_OPTIMUM_SIZE && height > TEMPLATE_FORM_OPTIMUM_SIZE)
    {
        ratio = (TEMPLATE_FORM_OPTIMUM_SIZE / (float)with + TEMPLATE_FORM_OPTIMUM_SIZE / (float)height) * 0.5;
    }

    else if (with > height)
    {
        ratio = (TEMPLATE_FORM_OPTIMUM_SIZE / (float)with + ((float)height) / TEMPLATE_FORM_OPTIMUM_SIZE) * 0.5;
    }
    else
    {
        ratio = (TEMPLATE_FORM_OPTIMUM_SIZE / (float)height + ((float)with) / TEMPLATE_FORM_OPTIMUM_SIZE) * 0.5;
    }
    return QSize(with * ratio, height * ratio);
}

void DockContainer::hideTemplateForm()
{
    if (_templateFormOnDrag != nullptr)
    {
        _templateFormOnDrag->hide();
    }
}

QWidget *DockContainer::widgetAt(QPoint p)
{
    if (_templateFormOnDrag != nullptr)
    {
        _templateFormOnDrag->setMask(QRegion(_templateFormOnDrag->rect()));
    }

    QWidget *currentWidget = QApplication::widgetAt(p);

    if (_templateFormOnDrag != nullptr)
    {
        _templateFormOnDrag->clearMask();
    }

    QWidgetList topWindowList;
    if (nullptr == currentWidget)
    {
        topWindowList = QApplication::topLevelWidgets();
    }
    else if (qobject_cast<TabBar *>(currentWidget) == nullptr)
    {
        QWidget *topLevelWindow = currentWidget;
        while (!topLevelWindow->isTopLevel() && topLevelWindow->parentWidget() != nullptr)
        {
            topLevelWindow = topLevelWindow->parentWidget();
        }
        topWindowList.append(topLevelWindow);
    }

    foreach(QWidget *widget, topWindowList)
    {
        if (!widget->isVisible())
        {
            continue;
        }

        if (widget == _templateFormOnDrag)
        {
            continue;
        }

        QRect expandRect = widget->frameGeometry().adjusted(-20, -20, 20, 20);
        QRect indenRect = widget->geometry().adjusted(20, 20, -20, -20);
        QRegion borderRegion = QRegion(expandRect) - QRegion(indenRect);
        if (borderRegion.contains(p))
        {
            return widget;
        }
    }
    return currentWidget;
}

DockContainer::RegionType DockContainer::getRegionType(QPoint pt, QRect rect)
{
    bool isInLeft = pt.x() < rect.width() / 3;
    bool isInRight = pt.x() > rect.width() * 2 / 3;
    bool isInTop = pt.y() < rect.height() / 3;
    bool isInBttom = pt.y() > rect.height() * 2 / 3;

    RegionType regionType;
    if (isInLeft)
    {
        regionType = LEFT;
        if (isInTop)
        {
            if (pt.y() < pt.x())
            {
                regionType = TOP;
            }
        }
        else if (isInBttom)
        {
            if (rect.height() - pt.y() < pt.x())
            {
                regionType = BOTTOM;
            }
        }
        return regionType;
    }

    if (isInRight)
    {
        RegionType regionType = RIGHT;
        if (isInTop)
        {
            if (pt.y() < rect.width() - pt.x())
            {
                regionType = TOP;
            }
        }
        else if (isInBttom)
        {
            if (pt.y() > pt.x())
            {
                regionType = BOTTOM;
            }
        }
        return regionType;
    }
    if (isInTop)
    {
        return TOP;
    }
    if (isInBttom)
    {
        return BOTTOM;
    }
    return CENTRAL;
}

void DockContainer::changeRootSplitter(Splitter *oldRootSplitter, Splitter *newRootSplitter)
{
    for (auto iter = _rootSplitterList.begin(); iter != _rootSplitterList.end(); iter++)
    {
        if (oldRootSplitter == *iter)
        {
            if (iter != _rootSplitterList.begin())
            {
                (*iter)->disconnect((*iter)->parent());
                connect(newRootSplitter, &Splitter::destroyed, oldRootSplitter->parent(), &QObject::deleteLater);
            }
            *iter = newRootSplitter;
            break;
        }
    }
}

void DockContainer::onSplitterDestroyed(QObject *obj)
{
    if (_isDisConnectAll)
    {
        return;
    }
    for (auto iter = _rootSplitterList.begin(); iter != _rootSplitterList.end(); iter++)
    {
        if (obj == *iter)
        {
            _rootSplitterList.erase(iter);
            break;
        }
    }
}

void DockContainer::onTabBarDestroyed(QObject *obj)
{
    if (_isDisConnectAll)
    {
        return;
    }
    QWidget *widget = qobject_cast<QWidget*>(obj);
    auto iter = _tabBarSet.find(widget);
    if (iter != _tabBarSet.end())
    {
        _tabBarSet.erase(iter);
    }
}

void DockContainer::onTabBarCloseTab()
{
    if (_isDisConnectAll)
    {
        return;
    }
    QWidget *removedWidget = _contextMenuTabWidget->removeTabAndWidget(_contextMenuTabIndex);
    if (_contextMenuTabWidget->widgetCount() == 0)
    {
        _contextMenuTabWidget->deleteLater();
    }
    if (nullptr != removedWidget)
    {
        removedWidget->deleteLater();
    }
}

void DockContainer::onAddTab(int windowType)
{
    auto window = _dockableWindowPool->newWindow(windowType);
    this->tabbedView(window, _contextMenuTabWidget, -1, window->getTitle());
}

void DockContainer::onDockableWindowDestroyed(QObject *obj)
{
    if (_isDisConnectAll)
    {
        return;
    }
    DockableWindow *w = static_cast<DockableWindow *>(obj);
    _dockableWindowPool->deleteWindow(w);
}

void DockContainer::onTabMaxmized()
{
    if (_maxmizedWindow == nullptr)
    {
        _maxmizedWindow = qobject_cast<DockableWindow*>(_contextMenuTabWidget->widget(_contextMenuTabIndex));
        _maxmizedWindowSourceTabIndex = _contextMenuTabIndex;
        _maxmizedWindowSourceTabWidget = _contextMenuTabWidget;
        _contextMenuTabWidget->removeOnlyWidget(_maxmizedWindow);

        if (_maxmizedTempTabWidget == nullptr)
        {
            _maxmizedTempTabWidget  = createTabWidget();
            _parentWidget->layout()->addWidget(_maxmizedTempTabWidget );
        }
        _maxmizedTempTabWidget ->addTab(_maxmizedWindow, _maxmizedWindow->getTitle());
        _maxmizedTempTabWidget->show();
        _dockRootWidget->hide();
    }
    else
    {
        _maxmizedTempTabWidget->removeTabAndWidget(0);
        _maxmizedWindowSourceTabWidget->insertOnlyWidget(_maxmizedWindowSourceTabIndex, _maxmizedWindow);
        _dockRootWidget->show();
        _maxmizedTempTabWidget->hide();

        _maxmizedWindow = nullptr;
        _maxmizedWindowSourceTabIndex = -1;
        _maxmizedWindowSourceTabWidget = nullptr;
    }
}

QWidget *DockContainer::rootWidgetAt(QPoint pt)
{
    for (int i = 0; i < _rootSplitterList.size(); i++)
    {
        if (i == 0)
        {
            if (_parentWidget->frameGeometry().contains(pt))
            {
                return _parentWidget;
            }

        }
        else if (_rootSplitterList[i]->frameGeometry().contains(pt))
        {
            return _rootSplitterList[i];
        }
    }
    return nullptr;
}

QWidget *DockContainer::rootWidgetof(QWidget *childWidget)
{
    Splitter *rootSplitter = getRootSplitter(childWidget);
    if (nullptr != rootSplitter)
    {
        if (rootSplitter == _rootSplitterList[0])
        {
            return _parentWidget;
        }
        return rootSplitter;
    }
    return nullptr;
}

void DockContainer::enableDrag(bool bEnable)
{
    _filterSwitch = bEnable;
}

void DockContainer::createDefaultLayout()
{
    //clear
    _dockableWindowPool->hideAllWindowsBeforeChangeLayout();
    _tabBarSet.clear();
    _rootSplitterList.clear();
    if (nullptr != _dockRootWidget)
    {
        _parentWidget->layout()->removeWidget(_dockRootWidget);
        delete _dockRootWidget;
    }
    _dockRootWidget = new QWidget();
    _parentWidget->layout()->addWidget(_dockRootWidget);
    _dockRootWidget->setObjectName("DockRootWidget");
    QHBoxLayout* carrierlaytout = new QHBoxLayout(_dockRootWidget);
    carrierlaytout->setSpacing(0);
    carrierlaytout->setContentsMargins(0, 0, 0, 0);

    //根分割窗口,水平方向
    Splitter *mainRootSplitter = createSplitterWidget();
    mainRootSplitter->setOrientation(Qt::Horizontal);
    connect(mainRootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    carrierlaytout->addWidget(mainRootSplitter);
    _rootSplitterList.append(mainRootSplitter);

    Splitter *splitter_1 = createSplitterWidget();
    splitter_1->setOrientation(Qt::Vertical);
    mainRootSplitter->addWidget(splitter_1);

    //默认窗口
    TabWidget *tabWidget_1_1 = createTabWidget();
    splitter_1->addWidget(tabWidget_1_1);
    DockableWindow *fiterWindow = _dockableWindowPool->newWindow();
    connect(fiterWindow, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
    tabWidget_1_1->addTab(fiterWindow, fiterWindow->getTitle());
}

}

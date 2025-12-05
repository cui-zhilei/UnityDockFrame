#include <QWidget>
#include <QEvent>
#include <QApplication>
#include <QLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QTabWidget>
//#include <QDesktopWidget>
#include <QMargins>
#include <QMenu>
#include <QSignalMapper>
#include <QJsonArray>

#include "DockContainer.h"
#include "TabBar.h"
#include "TabWidget.h"
#include "DockableWindow.h"
#include "DockableWindowPool.h"
#include "Splitter.h"
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

class DockContainerPrivate {
    Q_DECLARE_PUBLIC(DockContainer)
public:
    DockContainerPrivate(DockContainer *q)
        : q_ptr(q)
        , parentWidget(nullptr)
        , dockRootWidget(nullptr)
        , sourceTabWidget(nullptr)
        , sourceTabIndex(-1)
        , sourceView(nullptr)
        , filterSwitch(true)
        , isDragging(false)
        , templateFormOnDrag(nullptr)
        , contextMenuTabWidget(nullptr)
        , contextMenuTabIndex(-1)
        , maxmizedWindow(nullptr)
        , maxmizedWindowSourceTabWidget(nullptr)
        , maxmizedWindowSourceTabIndex(-1)
        , maxmizedTempTabWidget(nullptr)
        , isDisConnectAll(false)
        , dockableWindowPool(nullptr)
        , isDraggingCancelled(false)
    {}

    DockContainer *q_ptr;
    QWidget *parentWidget;
    QWidget *dockRootWidget;
    QList<Splitter *> rootSplitterList;
    QSet<QWidget *> tabBarSet;

    QPoint mousePressPos;
    TabWidget *sourceTabWidget;
    int sourceTabIndex;
    QWidget *sourceView;
    QString sourceTabText;

    DockContainer::HoverWidgetData hoverWidgetData;
    bool filterSwitch;
    bool isDragging;
    QTabWidget *templateFormOnDrag;

    TabWidget *contextMenuTabWidget;
    int contextMenuTabIndex;

    DockableWindow *maxmizedWindow;
    TabWidget *maxmizedWindowSourceTabWidget;
    int maxmizedWindowSourceTabIndex;
    TabWidget *maxmizedTempTabWidget;
    bool isDisConnectAll;

    DockableWindowPool *dockableWindowPool;
    bool isDraggingCancelled;
};

DockContainer::DockContainer(QWidget *parent)
    : QObject(parent)
    , d_ptr(new DockContainerPrivate(this))
{
    Q_D(DockContainer);
    d->parentWidget = parent;
    setObjectName("DockContainer");

    QHBoxLayout* laytout = new QHBoxLayout(d->parentWidget);
    laytout->setSpacing(0);
    laytout->setContentsMargins(0, 0, 0, 0);
    d->dockableWindowPool = new DockableWindowPool();
    initLayout();
}

DockContainer::~DockContainer()
{
    Q_D(DockContainer);
    d->isDisConnectAll = true;
    delete d_ptr;
}

void DockContainer::saveLayoutToJson(QJsonObject &jsonObj)
{
    Q_D(DockContainer);
    if (d->maxmizedWindow != nullptr)
    {
        onTabMaxmized();
    }
    QJsonArray floatWindowChildren;
    for (int i = 0; i < d->rootSplitterList.size(); i++)
    {
        QJsonObject windowObj;
        Splitter *rootSplitter = d->rootSplitterList[i];
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
    Q_D(DockContainer);
    d->tabBarSet.clear();
    d->rootSplitterList.clear();
    d->dockableWindowPool->hideAllWindowsBeforeChangeLayout();
    if (nullptr != d->dockRootWidget)
    {
        d->parentWidget->layout()->removeWidget(d->dockRootWidget);
        delete d->dockRootWidget;
    }
    d->dockRootWidget = new QWidget();
    d->dockRootWidget->setObjectName("DockRootWidget");

    QJsonArray childreList = jsonObj.value(c_strFloatWindowChildren).toArray();
    if (childreList.isEmpty())
    {
        return;
    }
    d->parentWidget->layout()->addWidget(d->dockRootWidget);

    QHBoxLayout* carrierlaytout = new QHBoxLayout(d->dockRootWidget);
    carrierlaytout->setSpacing(0);
    carrierlaytout->setContentsMargins(0, 0, 0, 0);
    Splitter *mainRootSplitter = createSplitterFromJson(childreList[0].toObject());
    connect(mainRootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    carrierlaytout->addWidget(mainRootSplitter);
    d->rootSplitterList.append(mainRootSplitter);

    for (int i = 1; i < childreList.count(); i++)
    {
        QJsonObject windowJsonObj = childreList[i].toObject();
        Splitter *floatRootSplitter = createSplitterFromJson(windowJsonObj);
        d->rootSplitterList.append(floatRootSplitter);

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(floatRootSplitter);

        QWidget *floatWindow = new QWidget(d->dockRootWidget);
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
    Q_D(DockContainer);
    jsonObj.insert(c_strWidgetType, SPLITTER);
    jsonObj.insert(c_strOrientation, splitter->orientation());

    int size = 0;
    if (d->rootSplitterList.contains(splitter))
    {
        size = 1;
    }
    else
    {
        Splitter *parentSplitter = getParentSplitter(splitter);
        if (parentSplitter != nullptr)
        {
            Qt::Orientation orientation = parentSplitter->orientation();
            if (orientation == Qt::Horizontal)
            {
                size = splitter->width();
            }
            else
            {
                size = splitter->height();
            }
        }
        else
        {
            size = splitter->width();
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
    Q_D(DockContainer);
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
        if (dockableWindow == nullptr)
        {
            continue;
        }
        uint wType = dockableWindow->windowType();
        QJsonObject childObj;
        childObj.insert(c_strWidgetType, VIEW);
        childObj.insert(c_strWindowType, QString::number(wType));
        //if is last modification todo
        {
            int wId = d->dockableWindowPool->windowID(dockableWindow);
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
    Q_D(DockContainer);
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
            dockableWindow = d->dockableWindowPool->newWindow(windowType);
        }
        else
        {
            dockableWindow = d->dockableWindowPool->getWindow(windowType, wId);
        }
        if (dockableWindow)
        {
            dockableWindow->load(childOject);
            dockableWindow->setMinimumSize(WIDGET_MIN_SIZE, WIDGET_MIN_SIZE);
            dockableWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            connect(dockableWindow, SIGNAL(destroyed(QObject *)), this, SLOT(onDockableWindowDestroyed(QObject *)));
            tabWidget->addTab(dockableWindow, dockableWindow->getTitle());
        }
    }
    tabWidget->setCurrentTabIndex(currentTabIndex);
    return tabWidget;
}

void DockContainer::tabbedView(DockableWindow *view, TabWidget *tabWidget, int index, const QString &label)
{
    Q_D(DockContainer);
    int actIndex = index;
    if (nullptr == tabWidget)
    {
        if (d->rootSplitterList.isEmpty() || d->rootSplitterList[0] == nullptr)
        {
            return;
        }
        TabWidget *rootTab = qobject_cast<TabWidget *>(d->rootSplitterList[0]->widget(0));
        if (rootTab == nullptr)
        {
            return;
        }
        actIndex = rootTab->insertTab(index, view, label);
        tabWidget = rootTab;
    }
    else
    {
        actIndex = tabWidget->insertTab(index, view, label);
    }
    if (tabWidget != nullptr)
    {
        tabWidget->setCurrentTabIndex(actIndex);
    }
    //save for searching
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setMinimumSize(WIDGET_MIN_SIZE, WIDGET_MIN_SIZE);
    d->dockableWindowPool->registerWindow(view);
    connect(view, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
}

TabWidget *DockContainer::floatView(DockableWindow *view, const QString& title)
{
    QPoint cusPos = QApplication::primaryScreen()->availableGeometry().center();
    return floatView(view, title, cusPos);
}

void DockContainer::floatView(uint nWindowType)
{
    Q_D(DockContainer);
    auto pDockableWindow = d->dockableWindowPool->newWindow(nWindowType);
    if (pDockableWindow != nullptr)
    {
        floatView(pDockableWindow, pDockableWindow->getTitle());
    }
}

void DockContainer::activeView(uint nWindowType)
{
    Q_D(DockContainer);
    if (d->dockableWindowPool->hasVisibleWindow(nWindowType))
    {
        auto pDockableWindow = d->dockableWindowPool->getOneExistedWindow(nWindowType);
        if (pDockableWindow != nullptr)
        {
            TabWidget *pParentTabWidget = getParentTabWidget(pDockableWindow);
            if (pParentTabWidget != nullptr)
            {
                pParentTabWidget->setCurrentWidget(pDockableWindow);
            }
        }
    }
    else
    {
        floatView(nWindowType);
    }
}

TabWidget *DockContainer::floatView(QWidget *view, const QString& title, QPoint cusPos)
{
    Q_D(DockContainer);
    Splitter *rootSplitter = createSplitterWidget();
    TabWidget *tabWidget = createTabWidget();
    tabWidget->addTab(view, title);
    rootSplitter->addWidget(tabWidget);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(rootSplitter);

    QWidget *floatWindow = new QWidget(d->dockRootWidget);
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
    d->dockableWindowPool->registerWindow(qobject_cast<DockableWindow*>(view));
    connect(view, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
    d->rootSplitterList.append(rootSplitter);
    connect(rootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    connect(rootSplitter, &Splitter::destroyed, floatWindow, &QWidget::deleteLater);
    return tabWidget;
}

void DockContainer::relocateFloatWindowGeometry(QWidget *floatWindow, QRect geometry)
{
    Q_D(DockContainer);
    QRect rect = geometry;
    if (d->sourceTabWidget != nullptr)
    {
        rect = d->sourceTabWidget->rect();
        rect.moveCenter(geometry.center());
    }
    rect = getNearestRectInDesktopRect(rect, rect.center());
    //add title bar height
    rect.moveTop(rect.top() + floatWindow->frameGeometry().height() - floatWindow->geometry().height());
    floatWindow->setGeometry(rect);
}

QRect DockContainer::getNearestRectInDesktopRect(QRect sourceRect, const QPoint& p)
{
    QRect desktopRect = QApplication::primaryScreen()->availableGeometry();
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
    Q_D(DockContainer);
    TabWidget *tabWidget = new TabWidget();
    tabWidget->tabBar()->installEventFilter(this);
    tabWidget->setAttribute(Qt::WA_DeleteOnClose);
    //save for searching
    d->tabBarSet.insert(tabWidget->tabBar());
    connect(tabWidget->tabBar(), &TabBar::destroyed, this, &DockContainer::onTabBarDestroyed);
    return tabWidget;
}

bool DockContainer::eventFilter(QObject *watched, QEvent *event)
{
    Q_D(DockContainer);
    if (d->filterSwitch && watched != this) //tarbars
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
                if (d->isDragging)
                {
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                    if (keyEvent->key() == Qt::Key_Escape)
                    {
                        d->isDraggingCancelled = true;
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
    Q_D(DockContainer);
    (void)watched;

    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
    d->mousePressPos = mouseEvent->globalPosition().toPoint();
}

void DockContainer::tabBarMouseReleaseEvent(QObject *watched, QEvent *event)
{
    Q_D(DockContainer);
    (void)watched;
    if (d->isDraggingCancelled)
    {
        d->isDraggingCancelled = false;
    }
    else
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent *>(event);
        endDragging(mouseEvent->globalPosition().toPoint());
    }
    for (auto iter = d->tabBarSet.begin(); iter != d->tabBarSet.end(); iter++)
    {
        QWidget *tabBar = *iter;
        if (tabBar != nullptr)
        {
            TabWidget *tabWidget = qobject_cast<TabWidget *>(tabBar->parentWidget());
            if (tabWidget != nullptr)
            {
                tabWidget->endDragging();
            }
        }
    }
    d->mousePressPos.setX(-1);
    d->mousePressPos.setY(-1);
}

void DockContainer::tabBarMouseMoveEvent(QObject *watched, QEvent *event)
{
    Q_D(DockContainer);
    QMouseEvent* mouseEvent = static_cast<QMouseEvent *>(event);
    if ((mouseEvent->globalPosition().toPoint() - d->mousePressPos).manhattanLength() > QApplication::startDragDistance())
    {
        TabBar *tabBar = qobject_cast<TabBar *>(watched);
        beginDragging(tabBar);
        if (d->isDragging)
        {
            showDragging(mouseEvent->globalPosition().toPoint());
        }
    }
}

void DockContainer::tabBarContextMenuEvent(QObject *watched, QEvent *event)
{
    Q_D(DockContainer);
    QContextMenuEvent *contexMenuEvent = static_cast<QContextMenuEvent*>(event);
    TabBar *tabBar = qobject_cast<TabBar *>(watched);
    d->contextMenuTabIndex = tabBar->tabAt(contexMenuEvent->pos());
    d->contextMenuTabWidget = getParentTabWidget(tabBar);
    if (d->contextMenuTabIndex >= 0 && d->contextMenuTabWidget != nullptr)
    {
        QMenu *menu = new QMenu();
        DockableWindow *view = qobject_cast<DockableWindow*>(d->contextMenuTabWidget->widget(d->contextMenuTabIndex));
        if (view != nullptr)
        {
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
}

void DockContainer::createMaxmizeAction(QMenu *parentMenu)
{
    Q_D(DockContainer);
    QAction* maxmizeTabAction = parentMenu->addAction(QStringLiteral("最大化"));
    if (d->maxmizedWindow != nullptr)
    {
        maxmizeTabAction->setCheckable(true);
        maxmizeTabAction->setChecked(true);
    }
    if (d->contextMenuTabWidget != d->maxmizedTempTabWidget)
    {
        if (!d->rootSplitterList.isEmpty() && getRootSplitter(d->contextMenuTabWidget) != d->rootSplitterList[0])
        {
            maxmizeTabAction->setEnabled(false);
        }
    }
    connect(maxmizeTabAction, &QAction::triggered, this, &DockContainer::onTabMaxmized);
}

void DockContainer::createCloseTabAction(QMenu *parentMenu, DockableWindow *view)
{
    Q_D(DockContainer);
    QAction* closeTabAciton = parentMenu->addAction(QStringLiteral("关闭"));
    if (d->maxmizedWindow != nullptr || !view->canClose())
    {
        closeTabAciton->setEnabled(false);
    }
    else if (isLastTabInMainWindow(d->contextMenuTabWidget->tabBar()))
    {
        closeTabAciton->setEnabled(false);
    }
    connect(closeTabAciton, &QAction::triggered, this, &DockContainer::onTabBarCloseTab);
}

void DockContainer::creatAddTabMenu(QMenu *parentMenu)
{
    Q_D(DockContainer);
    QSignalMapper *signalMapper = new QSignalMapper(parentMenu);
    QMenu *addTabMenu = parentMenu->addMenu(QStringLiteral("打开"));
    auto allFactorires = WindowFactoryManager::getInstance()->getAllFactorys();
    for (auto iter = allFactorires.begin(); iter != allFactorires.end(); iter++)
    {
        auto factory = iter->second;
        if (factory)
        {
            QAction* action = addTabMenu->addAction(factory->getTitle());
            if (factory->isUnique() && d->dockableWindowPool->hasVisibleWindow(iter->first))
            {
                // If the window is globally unique, disable the menu item
                action->setEnabled(false);
                continue;
            }
            connect(action, SIGNAL(triggered(bool)), signalMapper, SLOT(map()));
            signalMapper->setMapping(action, (uint)iter->first);
            if (d->contextMenuTabWidget == d->maxmizedTempTabWidget)
            {
                addTabMenu->setEnabled(false);
            }
        }
    }
    connect(signalMapper, &QSignalMapper::mappedInt, this, &DockContainer::onAddTab);
}

bool DockContainer::isLastTabInMainWindow(TabBar *tabBar)
{
    Q_D(DockContainer);
    if (tabBar->count() > 1)
    {
        return false;
    }
    if (!d->parentWidget->isAncestorOf(tabBar))
    {
        return false;
    }
    if (d->rootSplitterList.isEmpty() || getRootSplitter(tabBar) != d->rootSplitterList[0])
    {
        return false;
    }

    for (auto iter = d->tabBarSet.begin(); iter != d->tabBarSet.end(); iter++)
    {
        if (*iter == tabBar)
        {
            continue;
        }
        if (d->maxmizedTempTabWidget != nullptr)
        {
            if (*iter == d->maxmizedTempTabWidget->tabBar())
            {
                continue;
            }
        }
        if (d->rootSplitterList.isEmpty() || getRootSplitter(*iter) != d->rootSplitterList[0])
        {
            continue;
        }
        return false;
    }
    return true;
}

void DockContainer::beginDragging(TabBar *tabBar)
{
    Q_D(DockContainer);
    if (!d->isDragging)
    {
        if (d->maxmizedTempTabWidget != nullptr)
        {
            if (d->maxmizedTempTabWidget->tabBar() == tabBar)
            {
                return;
            }
        }
        if (isLastTabInMainWindow(tabBar))
        {
            return;
        }
        for (auto iter = d->tabBarSet.begin(); iter != d->tabBarSet.end(); iter++)
        {
            QWidget *tabBar = *iter;
            if (tabBar != nullptr)
            {
                TabWidget *tabWidget = qobject_cast<TabWidget *>(tabBar->parentWidget());
                if (tabWidget != nullptr)
                {
                    tabWidget->beginDragging();
                }
            }
        }
        d->sourceTabWidget = qobject_cast<TabWidget *>(tabBar->parent());
        QPoint localPoint = tabBar->mapFromGlobal(d->mousePressPos);
        d->sourceTabIndex = tabBar->tabAt(localPoint);
        if (d->sourceTabIndex >= 0)
        {
            d->sourceTabText = d->sourceTabWidget->tabText(d->sourceTabIndex);
            d->sourceView = d->sourceTabWidget->widget(d->sourceTabIndex);
            if (d->sourceView == nullptr)
            {
                return;
            }
            d->sourceTabWidget->removeOnlyTab(d->sourceTabIndex);
            d->isDragging = true;
            qApp->installEventFilter(this);
        }
    }
}

void DockContainer::endDragging(QPoint pos)
{
    Q_D(DockContainer);
    if (d->isDragging)
    {
        if (d->isDraggingCancelled && d->hoverWidgetData.type == TAB)
        {
                // When dragging to tab hotspot display state, cancellation is not allowed. Fixed crash bug in 2020.10
                return;
            }
        qApp->removeEventFilter(this);
        if (d->isDraggingCancelled && d->sourceTabWidget != nullptr && d->hoverWidgetData.horverWidget != d->sourceTabWidget->tabBar())
        {
            //if be cancelled
            endDragByCancelled();
        }
        else if (d->hoverWidgetData.type == FLOAT)
        {
            endDragByFloated(pos);
        }
        else
        {
            d->parentWidget->setUpdatesEnabled(false);
            if (d->hoverWidgetData.type == TAB)
            {
                endDragByTabbled(pos);
            }
            else if (d->hoverWidgetData.type == DOCK_AT_ROOT)
            {
                endDragByDockedAtRoot(pos);
            }
            else //if dock
            {
                endDragByDockedAtChild(pos);
            }
        }
        if (d->sourceTabWidget != nullptr && d->sourceTabWidget->widgetCount() == 0)
        {
            d->sourceTabWidget->deleteLater();
        }
        d->hoverWidgetData.type = FLOAT;
        d->hoverWidgetData.horverWidget = nullptr;
        d->isDragging = false;
        d->sourceTabIndex = -1;
        d->sourceTabText = "";
        d->sourceView = nullptr;
        d->sourceTabWidget = nullptr;
        hideTemplateForm();
        QApplication::processEvents();
        d->parentWidget->setUpdatesEnabled(true);
        qApp->removeEventFilter(this);
    }
}

void DockContainer::endDragByTabbled(QPoint pos)
{
    Q_D(DockContainer);
    TabBar *targetTabBar = qobject_cast<TabBar *>(d->hoverWidgetData.horverWidget);
    if (targetTabBar == nullptr)
    {
        return;
    }
    TabWidget *targetTabWidget = qobject_cast<TabWidget *>(targetTabBar->parent());
    if (targetTabWidget == nullptr || d->sourceTabWidget == nullptr || d->sourceView == nullptr)
    {
        return;
    }
    QPoint localPos = targetTabWidget->tabBar()->mapFromGlobal(pos);
    int index = targetTabWidget->tabBar()->tabAt(localPos);
    targetTabWidget->removeTempTab();
    d->sourceTabWidget->removeOnlyWidget(d->sourceView);
    int newIndex = targetTabWidget->insertTab(index, d->sourceView, d->sourceTabText);
    targetTabWidget->setCurrentTabIndex(newIndex);
    targetTabWidget->setCurrentWidgetIndex(newIndex);
}

void DockContainer::endDragByDockedAtRoot(QPoint pos)
{
    Q_D(DockContainer);
    if (d->sourceTabWidget == nullptr || d->sourceView == nullptr || d->hoverWidgetData.horverWidget == nullptr)
    {
        return;
    }
    TabWidget *newTabWidget = createTabWidget();
    d->sourceTabWidget->removeOnlyWidget(d->sourceView);
    newTabWidget->addTab(d->sourceView, d->sourceTabText);
    Splitter *splitter = d->hoverWidgetData.horverWidget->findChild<Splitter *>();
    if (splitter == nullptr)
    {
        Q_ASSERT(false);
        return;
    }

    QPoint locatPoint = d->hoverWidgetData.horverWidget->mapFromGlobal(pos);
    RegionType type = getRegionType(locatPoint, d->hoverWidgetData.horverWidget->rect());
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
    Q_D(DockContainer);
    if (d->hoverWidgetData.horverWidget == nullptr || d->sourceTabWidget == nullptr || d->sourceView == nullptr)
    {
        return;
    }
    TabWidget *hoverTabWidget = getParentTabWidget(d->hoverWidgetData.horverWidget);
    if (hoverTabWidget == nullptr)
    {
        return;
    }
    TabWidget *newTabWidget = createTabWidget();
    d->sourceTabWidget->removeOnlyWidget(d->sourceView);
    newTabWidget->addTab(d->sourceView, d->sourceTabText);
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
    Q_D(DockContainer);
    d->sourceTabWidget->removeOnlyWidget(d->sourceView);
    floatView(d->sourceView, d->sourceTabText, pos);
}

void DockContainer::endDragByCancelled()
{
    Q_D(DockContainer);
    if (d->sourceTabWidget == nullptr)
    {
        return;
    }
    d->sourceTabWidget->addTempTab(d->sourceTabIndex, d->sourceTabText);
    d->sourceTabWidget->setCurrentTabIndex(d->sourceTabIndex);
    if (d->hoverWidgetData.type == TAB && d->hoverWidgetData.horverWidget != nullptr)
    {
        TabBar *oldTargetBar = qobject_cast<TabBar *>(d->hoverWidgetData.horverWidget);
        if (oldTargetBar != nullptr)
        {
            TabWidget *oldHoverTabWidget = qobject_cast<TabWidget *>(oldTargetBar->parent());
            if (oldHoverTabWidget != nullptr)
            {
                oldHoverTabWidget->removeTempTab();
            }
        }
    }
    TabBar *sourceTabBar = d->sourceTabWidget->tabBar();
    if (sourceTabBar == nullptr || d->sourceTabIndex < 0)
    {
        return;
    }
    QPoint tabCenter = sourceTabBar->tabRect(d->sourceTabIndex).center();
    auto gloablPointer = sourceTabBar->mapToGlobal(tabCenter);
    QMouseEvent mouseEventRlease(
        QEvent::MouseButtonRelease,
        QPointF(tabCenter),
        gloablPointer,
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QCoreApplication::sendEvent(sourceTabBar, &mouseEventRlease);
}

Splitter *DockContainer::getParentSplitter(QWidget *widget)
{
    if (widget == nullptr)
    {
        return nullptr;
    }
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
    if (widget == nullptr)
    {
        return nullptr;
    }
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
    return rootParentSplitter;
}

TabWidget *DockContainer::getParentTabWidget(QWidget *widget)
{
    if (widget == nullptr)
    {
        return nullptr;
    }
    QWidget *parentWidget = widget->parentWidget();
    TabWidget *parentTabWidget = qobject_cast<TabWidget *>(parentWidget);
    while (parentTabWidget == nullptr && parentWidget != nullptr)
    {
        parentWidget = parentWidget->parentWidget();
        parentTabWidget = qobject_cast<TabWidget *>(parentWidget);
    }
    return parentTabWidget;
}

void DockContainer::showDragging(QPoint currrentCurPos)
{
    Q_D(DockContainer);
    HoverWidgetData hoverData;
    getHoverWidgetData(currrentCurPos, hoverData);
    if (hoverData.horverWidget == nullptr || hoverData.horverWidget != d->hoverWidgetData.horverWidget)
    {
        onHoverWidgetChanged(currrentCurPos, hoverData);
    }
    if (d->hoverWidgetData.type != hoverData.type)
    {
        d->hoverWidgetData.type = hoverData.type;
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
    Q_D(DockContainer);
    if (d->hoverWidgetData.type == TAB
        && d->hoverWidgetData.horverWidget->rect().contains(d->hoverWidgetData.horverWidget->mapFromGlobal(curPos)))
    {
        data = d->hoverWidgetData;
        return;
    }

    QWidget *hoverWidget = this->widgetAt(curPos);
    if (d->maxmizedWindow != nullptr)
    {
        if (d->parentWidget->isAncestorOf(hoverWidget)
            || hoverWidget->isAncestorOf(d->parentWidget))
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

    if (d->tabBarSet.contains(hoverWidget))
    {
        data.horverWidget = hoverWidget;
        data.type = TAB;
        return;
    }

    while (nullptr != hoverWidget)
    {
        if (d->dockableWindowPool->isDockedWindow(hoverWidget))
            break;
        hoverWidget = hoverWidget->parentWidget();
    }

    if (nullptr != hoverWidget)
    {
        TabWidget *tabWiget = getParentTabWidget(hoverWidget);
        if (tabWiget == d->sourceTabWidget && d->sourceTabWidget->widgetCount() == 1)
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
    Q_D(DockContainer);
    d->filterSwitch = false;

    if (d->hoverWidgetData.type == TAB)
    {
        TabBar *oldTargetBar = qobject_cast<TabBar *>(d->hoverWidgetData.horverWidget);
        QPoint tabCenter = oldTargetBar->tabRect(oldTargetBar->currentIndex()).center();
        auto gloablPointer = oldTargetBar->mapToGlobal(tabCenter);

        QMouseEvent mouseEventRlease(
            QEvent::MouseButtonRelease,
            QPointF(tabCenter),
            tabCenter,
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QCoreApplication::sendEvent(d->hoverWidgetData.horverWidget, &mouseEventRlease);

        TabWidget *oldHoverTabWidget = qobject_cast<TabWidget *>(oldTargetBar->parent());
        oldHoverTabWidget->removeTempTab();
    }

    if (newHoverdata.type == TAB)
    {
        TabBar *targetBar = qobject_cast<TabBar *>(newHoverdata.horverWidget);
        int tempTabIndex = targetBar->tabAt(targetBar->mapFromGlobal(curPos));
        TabWidget *targetTabWidget = qobject_cast<TabWidget *>(targetBar->parent());
        targetTabWidget->addTempTab(tempTabIndex, d->sourceTabText);
        targetBar->setAcceptMoveEvent(true);

        QPoint tabCenter;
        if (tempTabIndex < 0)
        {
            tempTabIndex = targetBar->count() - 1;
        }
        tabCenter = targetBar->tabRect(tempTabIndex).center();
        auto gloablPointer = targetBar->mapToGlobal(tabCenter);

        QMouseEvent mouseEventpress(
            QEvent::MouseButtonPress,
            QPointF(tabCenter),
            tabCenter,
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QCoreApplication::sendEvent(newHoverdata.horverWidget, &mouseEventpress);
    }
    if (d->hoverWidgetData.horverWidget == nullptr)
    {
        //when first time come here
        TabBar *oldTargetBar = qobject_cast<TabBar *>(d->sourceTabWidget->tabBar());
        {
            oldTargetBar->setAcceptMoveEvent(false);
        }
    }
    d->filterSwitch = true;
    d->hoverWidgetData = newHoverdata;
}

void DockContainer::whenDragOnTabBar(TabBar *targetTabBar, QPoint   targetPos)
{
    Q_D(DockContainer);
    hideTemplateForm();
    if (d->sourceTabWidget->tabBar() == targetTabBar)
    {
        return;
    }
    d->filterSwitch = false;
    QMouseEvent mouseEventMove(
        QEvent::MouseMove,
        QPointF(targetTabBar->mapFromGlobal(targetPos)),
        targetPos,
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QCoreApplication::sendEvent(targetTabBar, &mouseEventMove);
    d->filterSwitch = true;
}

void DockContainer::whenDragIngore(QPoint cursorPos)
{
    Q_D(DockContainer);
    createTemplateForm();
    d->templateFormOnDrag->resize(calcFloatTemplateFormSize());
    d->templateFormOnDrag->setWindowOpacity(0.8);
    QRect rect = d->templateFormOnDrag->rect();
    rect.moveCenter(cursorPos);
    rect = getNearestRectInDesktopRect(rect, cursorPos);

    d->templateFormOnDrag->move(rect.topLeft());
    d->templateFormOnDrag->raise();
    d->templateFormOnDrag->show();
}

void DockContainer::whenDragAcceptDock(QPoint cursorPos, bool isDockAtRoot)
{
    Q_D(DockContainer);
    createTemplateForm();
    QWidget *hoverTabWidget = d->hoverWidgetData.horverWidget;
    if (!isDockAtRoot)
    {
        hoverTabWidget = getParentTabWidget(d->hoverWidgetData.horverWidget);
    }
    else if(!d->rootSplitterList.isEmpty() && hoverTabWidget != nullptr && hoverTabWidget->isAncestorOf(d->rootSplitterList[0]))
    {
        hoverTabWidget = d->rootSplitterList[0];
    }
    if (hoverTabWidget == nullptr)
    {
        return;
    }
    QPoint locatPoint = hoverTabWidget->mapFromGlobal(cursorPos);
    RegionType type = getRegionType(locatPoint, hoverTabWidget->rect());
    if (type == CENTRAL)
    {
        d->hoverWidgetData.type = FLOAT;
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
        d->templateFormOnDrag->setGeometry(rect);
        d->templateFormOnDrag->setWindowOpacity(1.0);
        d->templateFormOnDrag->raise();
        d->templateFormOnDrag->show();
    }
}

void DockContainer::createTemplateForm()
{
    Q_D(DockContainer);
    if (d->templateFormOnDrag == nullptr)
    {
        d->templateFormOnDrag = new QTabWidget(nullptr);
        d->templateFormOnDrag->addTab(new QWidget(), d->sourceTabText);
        d->templateFormOnDrag->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
        d->templateFormOnDrag->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    d->templateFormOnDrag->setTabText(0, d->sourceTabText);
}

QSize DockContainer::calcFloatTemplateFormSize()
{
    Q_D(DockContainer);
    int with = d->sourceTabWidget->width();
    int height = d->sourceTabWidget->height();
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
    Q_D(DockContainer);
    if (d->templateFormOnDrag != nullptr)
    {
        d->templateFormOnDrag->hide();
    }
}

QWidget *DockContainer::widgetAt(QPoint p)
{
    Q_D(DockContainer);
    if (d->templateFormOnDrag != nullptr)
    {
        d->templateFormOnDrag->setMask(QRegion(d->templateFormOnDrag->rect()));
    }

    QWidget *currentWidget = QApplication::widgetAt(p);

    if (d->templateFormOnDrag != nullptr)
    {
        d->templateFormOnDrag->clearMask();
    }

    QWidgetList topWindowList;
    if (nullptr == currentWidget)
    {
        topWindowList = QApplication::topLevelWidgets();
    }
    else if (qobject_cast<TabBar *>(currentWidget) == nullptr)
    {
        QWidget *topLevelWindow = currentWidget;
        while (!topLevelWindow->isWindow() && topLevelWindow->parentWidget() != nullptr)
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

        if (widget == d->templateFormOnDrag)
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
    Q_D(DockContainer);
    for (auto iter = d->rootSplitterList.begin(); iter != d->rootSplitterList.end(); iter++)
    {
        if (oldRootSplitter == *iter)
        {
            if (iter != d->rootSplitterList.begin())
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
    Q_D(DockContainer);
    if (d->isDisConnectAll)
    {
        return;
    }
    for (auto iter = d->rootSplitterList.begin(); iter != d->rootSplitterList.end(); iter++)
    {
        if (obj == *iter)
        {
            d->rootSplitterList.erase(iter);
            break;
        }
    }
}

void DockContainer::onTabBarDestroyed(QObject *obj)
{
    Q_D(DockContainer);
    if (d->isDisConnectAll)
    {
        return;
    }
    QWidget *widget = qobject_cast<QWidget*>(obj);
    auto iter = d->tabBarSet.find(widget);
    if (iter != d->tabBarSet.end())
    {
        d->tabBarSet.erase(iter);
    }
}

void DockContainer::onTabBarCloseTab()
{
    Q_D(DockContainer);
    if (d->isDisConnectAll)
    {
        return;
    }
    QWidget *removedWidget = d->contextMenuTabWidget->removeTabAndWidget(d->contextMenuTabIndex);
    if (d->contextMenuTabWidget->widgetCount() == 0)
    {
        d->contextMenuTabWidget->deleteLater();
    }
    if (nullptr != removedWidget)
    {
        removedWidget->deleteLater();
    }
}

void DockContainer::onAddTab(int windowType)
{
    Q_D(DockContainer);
    auto window = d->dockableWindowPool->newWindow(windowType);
    this->tabbedView(window, d->contextMenuTabWidget, -1, window->getTitle());
}

void DockContainer::onDockableWindowDestroyed(QObject *obj)
{
    Q_D(DockContainer);
    if (d->isDisConnectAll)
    {
        return;
    }
    DockableWindow *w = static_cast<DockableWindow *>(obj);
    d->dockableWindowPool->deleteWindow(w);
}

void DockContainer::onTabMaxmized()
{
    Q_D(DockContainer);
    if (d->maxmizedWindow == nullptr)
    {
        d->maxmizedWindow = qobject_cast<DockableWindow*>(d->contextMenuTabWidget->widget(d->contextMenuTabIndex));
        if (d->maxmizedWindow == nullptr)
        {
            return;
        }
        d->maxmizedWindowSourceTabIndex = d->contextMenuTabIndex;
        d->maxmizedWindowSourceTabWidget = d->contextMenuTabWidget;
        d->contextMenuTabWidget->removeOnlyWidget(d->maxmizedWindow);

        if (d->maxmizedTempTabWidget == nullptr)
        {
            d->maxmizedTempTabWidget  = createTabWidget();
            d->parentWidget->layout()->addWidget(d->maxmizedTempTabWidget );
        }
        d->maxmizedTempTabWidget ->addTab(d->maxmizedWindow, d->maxmizedWindow->getTitle());
        d->maxmizedTempTabWidget->show();
        d->dockRootWidget->hide();
    }
    else
    {
        if (d->maxmizedTempTabWidget != nullptr)
        {
            d->maxmizedTempTabWidget->removeTabAndWidget(0);
            d->maxmizedTempTabWidget->hide();
        }
        if (d->maxmizedWindowSourceTabWidget != nullptr && d->maxmizedWindow != nullptr)
        {
            d->maxmizedWindowSourceTabWidget->insertOnlyWidget(d->maxmizedWindowSourceTabIndex, d->maxmizedWindow);
        }
        d->dockRootWidget->show();

        d->maxmizedWindow = nullptr;
        d->maxmizedWindowSourceTabIndex = -1;
        d->maxmizedWindowSourceTabWidget = nullptr;
    }
}

QWidget *DockContainer::rootWidgetAt(QPoint pt)
{
    Q_D(DockContainer);
    for (int i = 0; i < d->rootSplitterList.size(); i++)
    {
        if (i == 0)
        {
            if (d->parentWidget->frameGeometry().contains(pt))
            {
                return d->parentWidget;
            }

        }
        else if (d->rootSplitterList[i]->frameGeometry().contains(pt))
        {
            return d->rootSplitterList[i];
        }
    }
    return nullptr;
}

QWidget *DockContainer::rootWidgetof(QWidget *childWidget)
{
    Q_D(DockContainer);
    Splitter *rootSplitter = getRootSplitter(childWidget);
    if (nullptr != rootSplitter)
    {
        if (!d->rootSplitterList.isEmpty() && rootSplitter == d->rootSplitterList[0])
        {
            return d->parentWidget;
        }
        return rootSplitter;
    }
    return nullptr;
}

void DockContainer::enableDrag(bool bEnable)
{
    Q_D(DockContainer);
    d->filterSwitch = bEnable;
}

DockableWindow* DockContainer::getFirstVisibleWindow(uint type)
{
    Q_D(DockContainer);
    return d->dockableWindowPool->getFistVisibleWindow(type);
}

void DockContainer::initLayout()
{
    Q_D(DockContainer);
    //clear
    d->dockableWindowPool->hideAllWindowsBeforeChangeLayout();
    d->tabBarSet.clear();
    d->rootSplitterList.clear();
    if (nullptr != d->dockRootWidget)
    {
        d->parentWidget->layout()->removeWidget(d->dockRootWidget);
        delete d->dockRootWidget;
    }
    d->dockRootWidget = new QWidget();
    d->parentWidget->layout()->addWidget(d->dockRootWidget);
    d->dockRootWidget->setObjectName("DockRootWidget");
    QHBoxLayout* carrierlaytout = new QHBoxLayout(d->dockRootWidget);
    carrierlaytout->setSpacing(0);
    carrierlaytout->setContentsMargins(0, 0, 0, 0);

    //根分割窗口,水平方向
    Splitter *mainRootSplitter = createSplitterWidget();
    mainRootSplitter->setOrientation(Qt::Horizontal);
    connect(mainRootSplitter, &Splitter::destroyed, this, &DockContainer::onSplitterDestroyed);
    carrierlaytout->addWidget(mainRootSplitter);
    d->rootSplitterList.append(mainRootSplitter);

    Splitter *splitter_1 = createSplitterWidget();
    splitter_1->setOrientation(Qt::Vertical);
    mainRootSplitter->addWidget(splitter_1);

    //默认窗口
    TabWidget *tabWidget_1_1 = createTabWidget();
    splitter_1->addWidget(tabWidget_1_1);
    DockableWindow *fiterWindow = d->dockableWindowPool->newWindow();
    if (fiterWindow != nullptr)
    {
        connect(fiterWindow, &DockableWindow::destroyed, this, &DockContainer::onDockableWindowDestroyed);
        tabWidget_1_1->addTab(fiterWindow, fiterWindow->getTitle());
    }
}

}
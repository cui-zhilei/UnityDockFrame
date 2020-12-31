#include <QVBoxLayout>
#include <QStackedWidget>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>

#include <TabWidget.h>
#include <TabBar.h>
#include <DockableWindow.h>

namespace dock{

TabWidget::TabWidget(QWidget *parent)
    : QWidget(parent)
    , _tabBar(nullptr)
    , _stackedWidget(nullptr)
    , _oldCurrentIndexBeforAddTemp(-1)
{
    setObjectName("DockTabWidget");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 6, 0, 0);

    _tabBar = new TabBar(this);
    _tabBar->setMinimumHeight(20);
    _tabBar->setMovable(true);
    _tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _tabBar->setExpanding(false);
    _tabBar->installEventFilter(this);
    mainLayout->addWidget(_tabBar);

    _stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(_stackedWidget);

    connect(_tabBar, &TabBar::currentChanged, this, &TabWidget::setCurrentWidgetIndex);
}

TabWidget::~TabWidget()
{
}

TabBar* TabWidget::tabBar()
{
    return _tabBar;
}

int TabWidget::addTab(QWidget *page, const QString &label)
{
    _stackedWidget->addWidget(page);
    return _tabBar->addTab(label);
}

int TabWidget::insertTab(int index, QWidget *page, const QString &label)
{
    if (index < 0)
    {
        _stackedWidget->addWidget(page);
        return _tabBar->addTab(label);
    }
    _stackedWidget->insertWidget(index, page);
    return _tabBar->insertTab(index, label);
}

QString TabWidget::tabText(int index)
{
    Q_ASSERT(index >= 0 && index < _tabBar->count());
    return _tabBar->tabText(index);
}

QWidget *TabWidget::widget(int index)
{
    Q_ASSERT(index >= 0 && index < _stackedWidget->count());
    return _stackedWidget->widget(index);
}

QWidget *TabWidget::currentWidget()
{
    return _stackedWidget->currentWidget();
}

int TabWidget::widgetCount()
{
    return _stackedWidget->count();
}

void TabWidget::removeOnlyWidget(QWidget *widget)
{
    _stackedWidget->removeWidget(widget);
    widget->setParent(nullptr);
}

void TabWidget::insertOnlyWidget(int index, QWidget *page)
{
    _stackedWidget->insertWidget(index, page);
}

void TabWidget::removeOnlyTab(int index)
{
    Q_ASSERT(index >= 0 && index < _tabBar->count());
    _tabBar->removeTab(index);
}

void TabWidget::addTempTab(int index, QString& label)
{
    _oldCurrentIndexBeforAddTemp = _tabBar->currentIndex();
    if (index > -1)
    {
        _tabBar->insertTab(index, label);
    }
    else
    {
        _tabBar->addTab(label);
    }
}

QWidget *TabWidget::removeTabAndWidget(int index)
{
    QWidget *removedWidget = widget(index);
    removeOnlyTab(index);
    removeOnlyWidget(removedWidget);
    return removedWidget;
}

void TabWidget::removeTempTab()
{
    _tabBar->removeTab(_tabBar->currentIndex());
    setCurrentTabIndex(_oldCurrentIndexBeforAddTemp);
}

void TabWidget::beginDragging()
{
    disconnect(_tabBar, &TabBar::currentChanged, this, &TabWidget::setCurrentWidgetIndex);
}

void TabWidget::endDragging()
{
    connect(_tabBar, &TabBar::currentChanged, this, &TabWidget::setCurrentWidgetIndex);
}

void TabWidget::setCurrentTabIndex(int index)
{
    _tabBar->setCurrentIndex(index);
}

void TabWidget::setCurrentWidgetIndex(int index)
{
    _stackedWidget->setCurrentIndex(index);
}

void TabWidget::setCurrentWidget(QWidget* widget)
{
    auto index = _stackedWidget->indexOf(widget);
    if (index >= 0)
    {
        _tabBar->setCurrentIndex(index);
        _stackedWidget->setCurrentIndex(index);
    }
}

}

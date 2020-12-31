#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QWidget>
#include <QTabBar>
#include <QList>

class QStackedWidget;

namespace dock{
class TabBar;

class TabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget* parent = nullptr);
    virtual ~TabWidget();

    int addTab(QWidget* page, const QString& label);
    int insertTab(int index, QWidget *page, const QString& label);
    void setCurrentTabIndex(int index);
    int widgetCount();
    QString tabText(int index);
    QWidget* widget(int index);
    QWidget* currentWidget();
    TabBar* tabBar();

    void beginDragging();
    void endDragging();
    void addTempTab(int index, QString& label);
    void insertOnlyWidget(int index, QWidget *page);
    QWidget* removeTabAndWidget(int index);
    void removeOnlyTab(int index);
    void removeOnlyWidget(QWidget* widget);
    void removeTempTab();

    void setCurrentWidget(QWidget* widget);
public slots:
    void setCurrentWidgetIndex(int index);

private:
    TabBar*        _tabBar;
    QStackedWidget*      _stackedWidget;
    int                  _oldCurrentIndexBeforAddTemp;
};
}

#endif

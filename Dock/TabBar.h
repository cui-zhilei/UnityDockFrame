/**********************************************************
* @file        DockedTabBar.h
* @brief    停靠框架内可拖拽的TabBar
*
* @author    崔志雷
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef DOCKEDTABBAR_H
#define DOCKEDTABBAR_H

#include <QTabBar>
#include <QEvent>

namespace dock {

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    TabBar(QWidget *parent);
    virtual ~TabBar();

    void setAcceptMoveEvent(bool isAccept);
protected:
    virtual bool event(QEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);

private:
    bool _isAcceptMoveEvent;
};
}
#endif

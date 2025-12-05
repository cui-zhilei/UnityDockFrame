/**********************************************************
* @file        DockedTabBar.h
* @brief    Draggable TabBar within the docking framework
*
* @author    Cuizhilei
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

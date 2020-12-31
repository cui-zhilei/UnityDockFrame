/**********************************************************
* @file        DockedTabBar.h
* @brief    停靠框架内的分隔条控件
*
* @author    崔志雷
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef SPLITTERHANDLE_H
#define SPLITTERHANDLE_H
#include <QWidget>

namespace dock {

class SplitterHandle : public QWidget
{
    Q_OBJECT

public:
    explicit SplitterHandle(Qt::Orientation orientation, QWidget *parent);
    virtual ~SplitterHandle();
    void setOrientation(Qt::Orientation orientation);
private:
    Qt::Orientation _orientation;
};

}

#endif

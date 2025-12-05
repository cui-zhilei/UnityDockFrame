/**********************************************************
* @file        SplitterForDock.h
* @brief    Splitter control within the docking framework
*
* @author    Cuizhilei
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef SPLITTERFORDOCK_H
#define SPLITTERFORDOCK_H

#include <QWidget>
#include "dock_global.h"

namespace dock {

class SplitterHandle;
class Splitter : public QWidget
{
    Q_OBJECT

public:
    explicit Splitter(QWidget *parent = nullptr);
    virtual ~Splitter();
    int widgetCount() const;
    int indexOf(QWidget *w) const;
    
    QList<int> sizes() const;
    Qt::Orientation orientation() const { return _orientation; }

    void setOrientation(Qt::Orientation o);
    void setOpaqueResize(bool opaque = true);
    void insertWidget(int index, QWidget *w);
    void addWidget(QWidget *widget);
    QWidget *removeWidget(QWidget *widget);

    QWidget *widget(int index);
    QWidget *replaceWidget(int index, QWidget *widget);

    void updateSizes(const QList<int>& sizes);
    void updateSizes(int count, ...);
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;
    virtual void childEvent(QChildEvent *event) override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual bool event(QEvent *e) override;
    virtual QSize minimumSizeHint() const override;

private:
    int getSizeHint(const QWidget *w) const;
    int getSize(const QWidget *w) const;
    int getSize(const QRect &r) const;
    int getSize(const QSize &s) const;
    int getPos(const QRect &r) const;
    QList<int> getSizesByRects(const QList<QRect> &list) const;
    void resizeRect(QRect &r, int size) const;
    void moveRect(QRect &r, int pos) const;
    QRect getRectFromSize(int pos, int size) const;
    int handleCount() const;
    float sumFloats(const QList<float> &list) const;
    int sumInts(const QList<int> &list) const;
    int sumRects(const QList<QRect> &list) const;
    bool isMoveForward(QPoint posNow, QPoint posStart) const;

    void resizeChildren(const QList<QRect> &geoList);
    QList<QRect> recalcGeometries(const QList<float> &proprotions);

    QList<int> recalcSizesAtInserting(
        const QList<QRect> &oldGeo,
        int insertedIndex,
        int newsize,
        int minSize);

    QList<int> recalcSizesAtDeleting(const QList<QRect> &oldGeo, int deletedWidgetIndex);

    QList<int> recalcSizesAtMoving(
        const QList<QRect> &oldGeo,
        int movedHandleIndex,
        int moveDist,
        int minSize);

    QList<float> getProportions(const QList<int> &sizes, bool isContainHandleSizes);

    void onHandlePressEvent(SplitterHandle *h, QMouseEvent *e);
    void onHandleMoveEvent(SplitterHandle *h, QMouseEvent *e);
    void onHandleReleaseEvent(SplitterHandle *h, QMouseEvent *e);
private:
    int _handleWidth;
    int _minWidgetSize;
    QLayout *_layout;
    Qt::Orientation _orientation;
    QList<QWidget *> _widgetList;
    QList<SplitterHandle *> _handleList;
    QList<float> _sizeProportionArray;

    QPoint _startMovePos;
    QPoint _lastCurPos;
    QList<float> _lastSizeProportionsInMoving;
    bool _isMoveForwardSoonAgo;
    bool _isOpaqueResize;
    SplitterHandle *_floatHandle;
    QSize _minSizeHint;
};

}

#endif

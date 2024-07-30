#include <QMouseEvent>
#include <QApplication>
#include <QChildEvent>
#include "Splitter.h"
#include "SplitterHandle.h"

namespace dock {
static const QEvent::Type ENABLE_UPDATE_EVENT = (QEvent::Type)QEvent::registerEventType(QEvent::User + 200);
Splitter::Splitter(QWidget *parent)
    : QWidget(parent)
    , _handleWidth(4)
    , _minWidgetSize(200)
    , _layout(nullptr)
    , _orientation(Qt::Horizontal)
    , _isMoveForwardSoonAgo(true)
    , _isOpaqueResize(true)
    , _floatHandle(nullptr)
{
    setObjectName("SplitterForDock");
}

Splitter::~Splitter()
{

}

void Splitter::updateSizes(const QList<int>& sizes)
{
    _sizeProportionArray = getProportions(sizes, false);
    QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
    resizeChildren(geoList);
}

void Splitter::updateSizes(int count, ...)
{
    QList<int> sizeList;
    va_list argList;
    va_start(argList, count);
    for (int i = 0; i < count; i++)
    {
        int size = va_arg(argList, int);
        sizeList.append(size);
    }
    va_end(argList);
    updateSizes(sizeList);
}

QList<float> Splitter::getProportions(const QList<int>& sizes, bool isContainHandleSizes)
{
    QList<int> list;
    if (isContainHandleSizes)
    {
        for (int i = 0; i < sizes.size(); i++)
        {
            if (i % 2 != 0)
            {
                continue;
            }
            list.append(sizes.at(i));
        }
    }
    else
    {
        list = sizes;
    }

    float sumSizes = sumInts(list);
    QList<float> sizePropotions;
    sizePropotions.clear();
    for (int i = 0; i < list.size(); i++)
    {
        float size = (list.at(i) / sumSizes);
        sizePropotions.append(size);
    }
    return sizePropotions;
}

int Splitter::widgetCount() const
{
    return _widgetList.size();
}

int Splitter::indexOf(QWidget *w) const
{
    for (int i = 0; i < _widgetList.size(); i++)
    {
        if (w == _widgetList[i])
        {
            return i;
        }
    }
    return -1; 
}

QList<int> Splitter::sizes() const
{
    QList<int> sizes;
    for (int i = 0; i < _sizeProportionArray.size(); i++)
    {
        int size = (int)(_sizeProportionArray[i] * (getSize(this) - _handleWidth * handleCount()));
        sizes.append(size);
    }
    return sizes;
}

void Splitter::setOrientation(Qt::Orientation o)
{
    _orientation = o;
    for (int i = 0; i < _handleList.size(); i++)
    {
        _handleList[i]->setOrientation(o);
    }
}

void Splitter::setOpaqueResize(bool opaque)
{
    _isOpaqueResize = opaque;
}

void Splitter::insertWidget(int index, QWidget *w)
{
    w->setParent(this);
    if (_widgetList.empty())
    {
        _sizeProportionArray.append(1.0);
        _widgetList.append(w);
    }
    else
    {
        int sizeHint = getSizeHint(w);
        QList<QRect> oldGeo = recalcGeometries(_sizeProportionArray);
        QList<int> newSizes = recalcSizesAtInserting(oldGeo, index, sizeHint + 4, _minWidgetSize);
        newSizes.insert(index, 4);
        newSizes.insert(index + 1, sizeHint);
        _sizeProportionArray = getProportions(newSizes, true);
        SplitterHandle* h = new SplitterHandle(_orientation, this);
        h->setVisible(true);
        _handleList.insert(std::min<int>(index, _handleList.size()), h);
        _widgetList.insert(std::min<int>(index, _widgetList.size()), w);
    }
    if (w->isHidden())
    {
        w->setHidden(false);
    }
}

void Splitter::addWidget(QWidget *w)
{
    insertWidget(widgetCount(), w);
}

void Splitter::resizeChildren(const QList<QRect>& geoList)
{
    setUpdatesEnabled(false);
    for (int i = 0; i < geoList.size(); i++)
    {
        QRect newGeometry = geoList.at(i);
        if (i % 2 == 0)
        {
            int widgetIndex = i / 2;
            QWidget *w = _widgetList.at(widgetIndex);
            if (w->geometry() != newGeometry)
            {
                w->setGeometry(newGeometry);
            }
        }
        else
        {
            int handleIndex = (i + 1) / 2 - 1;
            SplitterHandle* h = _handleList.at(handleIndex);
            if (h->geometry() != newGeometry)
            {
                h->setGeometry(newGeometry);
            }
        }
    }
    int sumMinSize = _minWidgetSize * widgetCount();
    if (_orientation == Qt::Horizontal)
    {
        _minSizeHint = QSize(sumMinSize, _minWidgetSize);
    }
    else
    {
        _minSizeHint = QSize(_minWidgetSize, sumMinSize);
    }
    QApplication::postEvent(this, new QEvent(ENABLE_UPDATE_EVENT));
}

QWidget *Splitter::widget(int index)
{
    if (index < 0 || index >= _widgetList.size())
    {
        return nullptr;
    }
    return _widgetList.at(index);
}

QWidget *Splitter::replaceWidget(int index, QWidget *widget)
{
    if (index < 0 || index > _widgetList.size())
    {
        return nullptr;
    }
    widget->setParent(this);
    QWidget *oldw = _widgetList.at(index);
    _widgetList[index] = widget;
    oldw->setParent(nullptr);
    QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
    resizeChildren(geoList);
    if (widget->isHidden())
    {
        widget->setHidden(false);
    }
    return oldw;
}

inline int Splitter::getSizeHint(const QWidget *w) const
{
    QSize size = w->sizeHint();
    return (_orientation == Qt::Horizontal) ? size.width() : size.height();
}

inline int Splitter::getSize(const QWidget *w) const
{
    return (_orientation == Qt::Horizontal) ? w->width() : w->height();
}
int Splitter::getPos(const QRect& r) const
{
    return (_orientation == Qt::Horizontal) ? r.left() : r.top();
}

inline int Splitter::getSize(const QRect& r) const
{
    return (_orientation == Qt::Horizontal) ? r.width() : r.height();
}

inline int Splitter::getSize(const QSize& s) const
{
    if (!s.isValid())
    {
        return 0;
    }
    return (_orientation == Qt::Horizontal) ? s.width() : s.height();
}

void Splitter::resizeRect(QRect& r, int size) const
{
    (_orientation == Qt::Horizontal) ? r.setWidth(size) : r.setHeight(size);
}

void Splitter::moveRect(QRect& r, int pos) const
{
    (_orientation == Qt::Horizontal) ? r.moveLeft(pos) : r.moveTop(pos);
}

inline QList<int> Splitter::getSizesByRects(const QList<QRect>& list) const
{
    QList<int> sizeList;
    for (int i = 0; i < list.size(); i++)
    {
        sizeList.append(getSize(list[i]));
    }
    return sizeList;
}

QRect Splitter::getRectFromSize(int pos, int size) const
{
    QRect rect = this->rect();
    resizeRect(rect, size);
    moveRect(rect, pos);
    return rect;
}

inline int Splitter::handleCount() const
{
    return _handleList.size();
}

inline float Splitter::sumFloats(const QList<float> & list) const
{
    float sum = 0;
    for(int i = 0; i < list.size(); i++)
    {
        sum += list[i];
    }
    return sum;
}

inline int Splitter::sumInts(const QList<int> &list) const
{
    int sum = 0;
    for(int i = 0; i < list.size(); i++)
    {
        sum += list[i];
    }
    return sum;
}

inline int Splitter::sumRects(const QList<QRect> &list) const
{
    int sum = 0;
    for(int i = 0; i < list.size(); i++)
    {
        sum += getSize(list[i]);
    }
    return sum;
}

inline bool Splitter::isMoveForward(QPoint posNow, QPoint posStart) const
{
    QPoint moveVector = posNow - posStart;
    return (_orientation == Qt::Horizontal) ? (moveVector.x() > 0) : (moveVector.y() > 0);
}


QList<int> Splitter::recalcSizesAtInserting(
    const QList<QRect>& oldGeo,
    int insertedIndex, 
    int need,
    int minSize)
{
    insertedIndex = std::max<int>(insertedIndex, 0);
    insertedIndex = std::min<int>(insertedIndex, oldGeo.size() - 1);
    QList<int> sizeList = getSizesByRects(oldGeo);

    int sumDeduct = 0;
    int i = std::min<int>(2 * insertedIndex, oldGeo.size() - 1);
    while (sumDeduct < need)
    {
        const QRect& r = oldGeo[i];
        int s = getSize(r);
        if (s > minSize)
        {
            int deduct = std::min<int>(s - minSize, need - sumDeduct);
            sumDeduct += deduct;
            sizeList[i] = s - deduct;
        }
        i += 2;
        if (i < 0)
        {
            i += oldGeo.size();
        }
        i %= oldGeo.size();
        if (i == 2 * insertedIndex)
        {
            break;
        }
    }
    
    return  sizeList;
}

QList<int> Splitter::recalcSizesAtDeleting(const QList<QRect>& oldGeo, int deletedWidgetIndex)
{
    int indexGeo = deletedWidgetIndex * 2;
    QList<int> sizeList;
    for (int i = 0; i < oldGeo.size(); i++)
    {
        if (indexGeo == oldGeo.size() - 1)
        {
            if (i < oldGeo.size() - 2)
            {
                sizeList.append(getSize(oldGeo[i]));
            }
        }
        else
        {
            if (i != indexGeo && i != indexGeo + 1)
            {
                sizeList.append(getSize(oldGeo[i]));
            }
        }
    }

    if (deletedWidgetIndex == widgetCount() - 1)
    {
        sizeList[sizeList.size() - 1] += getSize(oldGeo[oldGeo.size() - 1]) + 4;
    }
    else
    {
        sizeList[indexGeo] += getSize(oldGeo[indexGeo]) + 4;
    }

    return  sizeList;
}

QList<int> Splitter::recalcSizesAtMoving(const QList<QRect>& oldGeo, int handleIndex, int moveDist, int minSize)
{
    QList<int> sizeList = getSizesByRects(oldGeo);

    int compressWidgtIndex = (moveDist > 0) ? handleIndex + 1: handleIndex;

    int sumDeduct = 0;
    int start = 2 * compressWidgtIndex;
    while (sumDeduct < abs(moveDist))
    {
        const QRect& r = oldGeo[start];
        int s = getSize(r);
        QWidget *w = _widgetList[start / 2];
        int widgetMinSize = getSize(w->minimumSize());
        int widgetMinSizeHint = getSize(w->minimumSizeHint());
        minSize = std::max<int>(minSize, widgetMinSize);
        minSize = std::max<int>(minSize, widgetMinSizeHint);
        if (s > minSize)
        {
            int deduct = std::min<int>(s - minSize, (abs)(moveDist) - sumDeduct);
            sumDeduct += deduct;
            sizeList[start] = s - deduct;
        }
        (moveDist > 0) ? start += 2 : start -= 2;
        if (start < 0 || start >= oldGeo.size())
        {
            break;
        }
    }
    int expandWidgetIndex = (moveDist > 0) ? (compressWidgtIndex - 1) : (compressWidgtIndex + 1);
    sizeList[2 * expandWidgetIndex] += sumDeduct;
    
    return  sizeList;
}

QList<QRect> Splitter::recalcGeometries(const QList<float>& proprotions)
{
    QList<QRect> list;
    int pos = 0;
    int totalWidgetSize = getSize(this) - handleCount() * _handleWidth;
    for (int i = 0; i < handleCount() + widgetCount(); i++)
    {
        int size = 0;
        if (i % 2 == 0)
        {
            int widgetIndex = i / 2;
            float prop = proprotions.at(widgetIndex);
            size = totalWidgetSize * prop;
            QWidget *w = _widgetList[widgetIndex];
            size = std::max<int>(size, getSize(w->minimumSize()));
            if (w->layout() != nullptr)
            {
                size = std::max<int>(size, getSize(w->minimumSizeHint()));
            }
        }
        else
        {
            size = _handleWidth;
        }
        QRect rect;
        if (Qt::Horizontal == _orientation)
        {
            rect = QRect(pos, 0, size, this->height());
        }
        else
        {
            rect = QRect(0, pos, this->width(), size);
        }
        pos += size;
        list.append(rect);
    }
    return list;
}

void Splitter::resizeEvent(QResizeEvent *event)
{
    QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
    resizeChildren(geoList);
    QWidget::resizeEvent(event);
}

void Splitter::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
}

bool Splitter::eventFilter(QObject *watched, QEvent *event)
{
    SplitterHandle* h = qobject_cast<SplitterHandle*>(watched);
    if (h != nullptr)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(event);
            onHandlePressEvent(h, e);
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(event);
            if (e->buttons() & Qt::LeftButton)
            {
                onHandleMoveEvent(h, e);
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            onHandleReleaseEvent(h, mouseEvent);
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool Splitter::event(QEvent* e)
{
    if (e->type() == ENABLE_UPDATE_EVENT)
    {
        setUpdatesEnabled(true);
        return true;
    }
    return QWidget::event(e);
}

void Splitter::onHandlePressEvent(SplitterHandle* h, QMouseEvent* e)
{
    (void)(h);
    _startMovePos = e->globalPosition().toPoint();
    _lastCurPos = e->globalPosition().toPoint();
}

void Splitter::onHandleMoveEvent(SplitterHandle* h, QMouseEvent* e)
{
    if (_startMovePos.x() < 0 || _startMovePos.y() < 0)
    {
        return;
    }
    int handIndex = _handleList.indexOf(h);
    if (handIndex == -1)
    {
        Q_ASSERT(false);
        return;
    }
    if (_isOpaqueResize)
    {
        bool moveForward = isMoveForward(e->globalPosition().toPoint(), _lastCurPos);
        if (_isMoveForwardSoonAgo != moveForward && !_lastSizeProportionsInMoving.isEmpty())
        {
            _sizeProportionArray = _lastSizeProportionsInMoving;
            _startMovePos = _lastCurPos;
        }
        QPoint moveVector = e->globalPosition().toPoint() - _startMovePos;
        int moveDist = (_orientation == Qt::Horizontal) ? moveVector.x() : moveVector.y();
        QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
        QList<int> newSizes = recalcSizesAtMoving(geoList, handIndex, moveDist, _minWidgetSize);
        _lastSizeProportionsInMoving = getProportions(newSizes, true);
        QList<QRect> newGeoList = recalcGeometries(_lastSizeProportionsInMoving);
        resizeChildren(newGeoList);
        _lastCurPos = e->globalPosition().toPoint();

        _isMoveForwardSoonAgo = moveForward;
    }
    else
    {
        QPoint moveVector = e->globalPosition().toPoint() - _startMovePos;
        int moveDist = (_orientation == Qt::Horizontal) ? moveVector.x() : moveVector.y();
        QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
        QList<int> newSizes = recalcSizesAtMoving(geoList, handIndex, moveDist, _minWidgetSize);
        _lastSizeProportionsInMoving = getProportions(newSizes, true);
        QList<QRect> newGeoList = recalcGeometries(_lastSizeProportionsInMoving);
        QRect handleGeometry = newGeoList.at(2 * handIndex + 1);
        handleGeometry.moveTo(this->mapToGlobal(handleGeometry.topLeft()));
        if (_floatHandle == nullptr)
        {
            _floatHandle = new SplitterHandle(_orientation, nullptr);
            _floatHandle->raise();
            _floatHandle->show();
        }
        _floatHandle->setGeometry(handleGeometry);
    }
}

void Splitter::onHandleReleaseEvent(SplitterHandle* h, QMouseEvent* e)
{
    (void*)(h);
    (void*)(e);
    _startMovePos.setX(-1);
    _startMovePos.setY(-1);
    _lastCurPos.setX(-1);
    _lastCurPos.setY(-1);
    if (!_isOpaqueResize)
    {
        if (nullptr != _floatHandle)
        {
            delete _floatHandle;
            _floatHandle = nullptr;
        }
    }
    if (!_lastSizeProportionsInMoving.isEmpty())
    {
        _sizeProportionArray = _lastSizeProportionsInMoving;
        _lastSizeProportionsInMoving.clear();
    }
    QList<QRect> newGeoList = recalcGeometries(_sizeProportionArray);
    resizeChildren(newGeoList);
}

void Splitter::childEvent(QChildEvent *event)
{
    if (event->type() == QEvent::ChildRemoved)
    {
        QWidget *w = static_cast<QWidget*>(event->child());
        removeWidget(w);
        if (widgetCount() == 0)
        {
            this->deleteLater();
        }
    }
    QWidget::childEvent(event);
}

QWidget *Splitter::removeWidget(QWidget *w)
{
    QWidget *ret = nullptr;
    int index = _widgetList.indexOf(w);
    if (index == -1)
    {
        return nullptr;
    }
    if (_widgetList.size() > 1)
    {
        QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
        QList<int> newSizes = recalcSizesAtDeleting(geoList, index);
        _sizeProportionArray = getProportions(newSizes, true);
    }

    ret = _widgetList.takeAt(index);
    Q_ASSERT(ret == w);
    if (!_handleList.isEmpty())
    {
       SplitterHandle* h = _handleList.takeAt(_handleList.size() - 1);
       h->deleteLater();
    }
    QList<QRect> geoList = recalcGeometries(_sizeProportionArray);
    resizeChildren(geoList);
    return ret;
}

QSize Splitter::minimumSizeHint() const
{
    QSize minSize = _minSizeHint;
    QSize needSize;
    for (int i = 0; i < _widgetList.size(); i++)
    {
        QSize s = _widgetList[i]->minimumSize();
        s = s.expandedTo(_widgetList[i]->minimumSizeHint());
        if (s.isValid())
        {
           needSize = needSize.expandedTo(s);
        }
    }
    minSize = minSize.expandedTo(needSize);
    return minSize;
}

}

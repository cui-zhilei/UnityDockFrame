#include <SplitterHandle.h>
namespace dock {

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent)
{
    this->setMouseTracking(true);
    setOrientation(orientation);
    this->installEventFilter(parent);
    this->setWindowOpacity(0.8);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
    this->setAttribute(Qt::WA_X11DoNotAcceptFocus);
    this->setAttribute(Qt::WA_ShowWithoutActivating);
}

SplitterHandle::~SplitterHandle()
{
}

void SplitterHandle::setOrientation(Qt::Orientation orientation)
{
    if (orientation == Qt::Vertical)
    {
        this->setCursor(Qt::SplitVCursor);
    }
    else
    {
        this->setCursor(Qt::SplitHCursor);
    }
    _orientation = orientation;
}

}

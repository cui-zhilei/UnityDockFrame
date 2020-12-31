#ifndef DOCK_GLOBAL_H
#define DOCK_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DOCK_LIBRARY)
#  define DOCKSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DOCKSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // DOCK_GLOBAL_H

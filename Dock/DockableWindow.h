/**********************************************************
* @file        DockableWindow.h
* @brief    Base class for dockable windows
*
* @author    Cuizhilei
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef DOCKABLE_H
#define DOCKABLE_H
#include "dock_global.h"

#include <QWidget>
#include <QJsonObject>
#include <QDebug>

#include "WindowFactoryManager.h"

// Declare window factory
//@param ClassName: Class name
//@param Title: Default window title
//@param IsUnique: Whether the window can only have one instance
#define DEC_WINDOW_FACTORY(ClassName, Title, IsUnique)							\
class ClassName##Factory : public dock::WindowFactory							\
{																				\
public:																			\
    ClassName##Factory()														\
    {																			\
        auto pManager = dock::WindowFactoryManager::getInstance();					\
        pManager->registerFactory(qHash(QString(#ClassName)), this); 			\
    }																			\
    virtual bool isUnique() override { return IsUnique; }						\
    virtual QString getTitle() override { return QStringLiteral(Title); }  		\
    virtual dock::DockableWindow* create(QWidget* p) override							\
    {																			\
        return new ClassName(p); 												\
    }                                                                           \
};

// Static registration macro, if automatic registration is not needed, use DEC_WINDOW_FACTORY and REGISTER_WINDOW
//@param ClassName: Class name
//@param Title: Default window title
//@param IsUnique: Whether the window can only have one instance
#define STATIC_REGISTER_WINDOW(ClassName, Title, IsUnique)						\
        DEC_WINDOW_FACTORY(ClassName, Title, IsUnique)							\
static ClassName##Factory g_##ClassName##FactoryInstance;

#define REGISTER_WINDOW(ClassName)												\
        WindowFactoryManager::getInstance()->registerFactory(qHash(QString(#ClassName)), \
        new ClassName##Factory, true);

#define WINDOW_TYPE_ID(ClassName)  qHash(QString(#ClassName)

class QMenu;

namespace dock {

class DOCKSHARED_EXPORT DockableWindow : public QWidget
{
    Q_OBJECT

public:
    DockableWindow(QWidget *parent = Q_NULLPTR);
    virtual ~DockableWindow();
    virtual void onFloating() {}
    virtual void onDocking() {}
    virtual void onContextMenu(QMenu* menu) { (void)menu; }
    virtual bool canClose() {return true;}
    virtual bool load(const QJsonObject &jsonObj) { (void)jsonObj; return true; }
    virtual void saveObject(QJsonObject &jsonObj)
    {
        //todo test code
        jsonObj.insert("DockableWindow", "null");
    }

    virtual QString getTitle();
    int windowType();
};


}


#endif // DOCKABLE_H

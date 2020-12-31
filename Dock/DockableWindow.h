/**********************************************************
* @file        DockableWindow.h
* @brief    可停靠窗口基类
*
* @author    崔志雷
* @date     2017.4
* @version  1.0.0
*
***********************************************************/
#ifndef DOCKABLE_H
#define DOCKABLE_H

#include <QWidget>
#include <QJsonObject>
#include <QDebug>

#include <WindowFactoryManager.h>

//声明窗口工厂
//@param ClassName:类名称
//@param ClassName:窗口默认标题
//@param IsUnique: 窗口是否只能有一个实例
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

//静态注册宏,如果不需要自动注册，可使用DEC_WINDOW_FACTORY和REGISTER_WINDOW
//@param ClassName:类名称
//@param ClassName:窗口默认标题
//@param IsUnique: 窗口是否只能有一个实例
#define STATIC_REGISTER_WINDOW(ClassName, Title, IsUnique)						\
        DEC_WINDOW_FACTORY(ClassName, Title, IsUnique)							\
static ClassName##Factory g_##ClassName##FactoryInstance;

#define REGISTER_WINDOW(ClassName)												\
        WindowFactoryManager::getInstance()->registerFactory(qHash(QString(#ClassName)), \
        new ClassName##Factory, true);

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

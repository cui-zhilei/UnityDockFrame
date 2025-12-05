#include <QMenuBar>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>

#include "MainWindow.h"
#include "DockContainer.h"
#include "DockableWindow.h"
#include "BlackWindow.h"
#include "WindowFactoryManager.h"

using namespace dock;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    //When using dynamic registration, call this macro
    //REGISTER_WINDOW(BlackWindow);

    this->setCentralWidget(new QWidget());
    m_pContainer = new DockContainer(this->centralWidget());

    QMenuBar *pMenuBar = menuBar();
    pMenuBar->addMenu(QStringLiteral("Menu 1"));

    QMenu *pWindowMenu = pMenuBar->addMenu(QStringLiteral("Window"));
    auto factories = dock::WindowFactoryManager::getInstance()->getAllFactorys();
    for (auto it = factories.begin(); it != factories.end(); it++)
    {
        dock::WindowFactory *pFac = it->second;
        QAction *pAction = pWindowMenu->addAction(pFac->getTitle());
        connect(pAction, &QAction::triggered, [this, windowType = it->first]() {
            this->onCreateWindow(windowType);
            });
    }

    QMenu *pLayoutMenu = pMenuBar->addMenu(QStringLiteral("Layout"));
    QAction *pActionLayout1 = pLayoutMenu->addAction(QStringLiteral("Layout 1"));
    connect(pActionLayout1, &QAction::triggered, this, &MainWindow::onLayout1);
    QAction *pActionLayout2 =pLayoutMenu->addAction(QStringLiteral("Layout 2"));
    connect(pActionLayout2, &QAction::triggered, this, &MainWindow::onLayout2);
    QAction *pActionSaveLayout =pLayoutMenu->addAction(QStringLiteral("Save Layout"));
    connect(pActionSaveLayout, &QAction::triggered, this, &MainWindow::onSaveLayout);
    QAction *pActionOpenLayout =pLayoutMenu->addAction(QStringLiteral("Open Layout"));
    connect(pActionOpenLayout, &QAction::triggered, this, &MainWindow::onOpenLayout);
    QAction *pActionDefaultLayout =pLayoutMenu->addAction(QStringLiteral("Default Layout"));
    connect(pActionDefaultLayout, &QAction::triggered, this, &MainWindow::onDefaultLayout);

    QAction *pActionFixLayout =pLayoutMenu->addAction(QStringLiteral("Fix Layout"));
    pActionFixLayout->setCheckable(true);
    connect(pActionFixLayout, &QAction::triggered, this, &MainWindow::onFixLayout);

    QString fileName = QApplication::applicationDirPath() + "/layout/lastModify.json";
    openLayout(fileName);
}

MainWindow::~MainWindow()
{
    QString fileName = QApplication::applicationDirPath() + "/layout/lastModify.json";
    saveLayout(fileName);
}

void MainWindow::onLayout1()
{
    QString fileName = QApplication::applicationDirPath() + "/layout/layout1.json";
    openLayout(fileName);
}

void MainWindow::onLayout2()
{
    QString fileName = QApplication::applicationDirPath() + "/layout/layout2.json";
    openLayout(fileName);
}

void MainWindow::onOpenLayout()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    openLayout(fileName);
}

void MainWindow::onSaveLayout()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    saveLayout(fileName);
}

void MainWindow::onDefaultLayout()
{
    m_pContainer->initLayout();
}

void MainWindow::onFixLayout(bool bChecked)
{
    m_pContainer->enableDrag(!bChecked);
}

void MainWindow::onCreateWindow(int windowType)
{
    m_pContainer->activeView(windowType);
}

void MainWindow::saveLayout(const QString &strFileName)
{
    QFile file(strFileName);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QJsonObject object;
    m_pContainer->saveLayoutToJson(object);

    QJsonDocument doc;
    doc.setObject(object);
    file.write(doc.toJson());
    file.close();
}

void MainWindow::openLayout(const QString &strFileName)
{
    QFile file(strFileName);
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_pContainer->createLayoutFromJson(doc.object());
    file.close();
}

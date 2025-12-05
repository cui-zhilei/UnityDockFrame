#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace dock {
    class DockContainer;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onLayout1();
    void onLayout2();
    void onOpenLayout();
    void onSaveLayout();
    void onDefaultLayout();
    void onFixLayout(bool bChecked);
    void onCreateWindow(int windowType);

private:
    void openLayout(const QString &strFileName);
    void saveLayout(const QString &strFileName);

private:
     dock::DockContainer *m_pContainer = nullptr;
};

#endif // MAINWINDOW_H

#pragma once
#include "NavigationPane.h"
#include "TargetDevice.h"

#include <QMainWindow>
#include <QTableWidgetItem>

#include <memory>

namespace Ui {
class CMainWindow;
}

class CMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CMainWindow(QWidget *parent = nullptr);
    ~CMainWindow();

protected:
    void InitTargetToolbar();
    void UpdateTarget(QString targetStr);

    void LoadClickConfig(const std::string &config, const std::string &list);
    void LoadPathHistogram(const std::string &pathHistogram);

private slots:
    void on_actionOpen_triggered();
    void on_actionLoadPathHistogram_triggered();
    void on_actionShowAll_triggered();
    void on_actionShowSelectionOnly_triggered();
    void on_pathTable_itemDoubleClicked(QTableWidgetItem *item);

private:
    Ui::CMainWindow *ui;
    CNavigationPane *NavigationPane;
    std::unique_ptr<CTargetDevice> TargetDevice;
};

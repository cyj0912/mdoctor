#include "MainWindow.h"
#include "FlatConfigToScene.h"
#include "GraphicsView.h"
#include "TraceEventClick.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsTextItem>
#include <QLabel>
#include <QMessageBox>
#include <QPixmapCache>
#include <QPushButton>
#include <QTextStream>
#include <QToolButton>
#include <QtDebug>

#include <fstream>
#include <iostream>
#include <sstream>

#define OBJ_GRAPHICS_VIEW "graphicsView"

CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CMainWindow)
{
    ui->setupUi(this);
    ui->navigationDock->setWidget(NavigationPane = new CNavigationPane());
    InitTargetToolbar();

    connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::closeAllWindows);

    QPixmapCache::setCacheLimit(1048576);

    auto *view = new CGraphicsView();
    view->setObjectName(OBJ_GRAPHICS_VIEW);
    ui->verticalLayout->addWidget(view);

    NavigationPane->SetView(view);

    ui->pathTable->setColumnCount(2);
    QStringList headings;
    headings << "Elements"
             << "Count";
    ui->pathTable->setHorizontalHeaderLabels(headings);

#if 1
    QToolButton *loadTraceDat = new QToolButton();
    loadTraceDat->setText("trace.dat");
    ui->mainToolBar->addWidget(loadTraceDat);
    connect(loadTraceDat, &QToolButton::clicked, [=](bool) {
        auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
        auto *scene = dynamic_cast<CClickGraphScene *>(view->scene());
        if (!scene) {
            ui->statusBar->showMessage("No click config loaded, cannot proceed.");
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open trace.dat"),
                                                        "",
                                                        tr("trace.dat (trace.dat);;All Files (*)"));
        if (!QFile::exists(fileName)) {
            ui->statusBar->showMessage(tr("Could not find file"));
            return;
        }
        CTraceDat parser(fileName);
        qDebug("bIsLoaded=%d", parser.IsLoaded());
        if (!parser.IsLoaded()) {
            ui->statusBar->showMessage("trace.dat loading failed");
            return;
        }
        auto *eventConsumer = new CPacketTraceModel(scene);
        parser.ProcessRecords(*eventConsumer);
        auto *model = new CPacketTraceSorter();
        model->setSourceModel(eventConsumer);

        // Setup filter box
        ui->packetsFilter->clear();
        connect(ui->packetsFilter, &QLineEdit::editingFinished, [this]() {
            auto *sorter = static_cast<CPacketTraceSorter *>(ui->packetsTable->model());
            sorter->SetFilterString(ui->packetsFilter->text());
        });

        ui->packetsTable->setModel(model);
    });
#endif
}

CMainWindow::~CMainWindow()
{
    delete ui;
}

void CMainWindow::InitTargetToolbar()
{
    QLabel *targetLable = new QLabel("Target:");
    ui->targetToolBar->addWidget(targetLable);

    QComboBox *targetSel = new QComboBox();
    targetSel->setEditable(true);
    ui->targetToolBar->addWidget(targetSel);

    QPushButton *fetchConfig = new QPushButton();
    fetchConfig->setText(tr("Fetch flatconfig"));
    connect(fetchConfig, &QPushButton::clicked, [=](bool) {
        this->UpdateTarget(targetSel->currentText());
        if (TargetDevice) {
            std::string empty;
            this->LoadClickConfig(TargetDevice->FetchFlatConfig(), empty);
        }
    });
    ui->targetToolBar->addWidget(fetchConfig);

    QPushButton *fetchHistogram = new QPushButton();
    fetchHistogram->setText(tr("Fetch path_histogram"));
    connect(fetchHistogram, &QPushButton::clicked, [=](bool) {
        if (TargetDevice) {
            this->LoadPathHistogram(TargetDevice->FetchPathHistogram());
        } else {
            ui->statusBar->showMessage("No target");
        }
    });
    ui->targetToolBar->addWidget(fetchHistogram);
}

void CMainWindow::UpdateTarget(QString targetStr)
{
    if (!TargetDevice) {
        TargetDevice = std::make_unique<CTargetDevice>(targetStr);
        if (!TargetDevice->IsValid() || !TargetDevice->SanityCheck()) {
            ui->statusBar->showMessage("Target invalid!");
            TargetDevice = nullptr;
        }
        return;
    }

    if (*TargetDevice == targetStr)
        return;

    //TODO
    ui->statusBar->showMessage("TODO: Already has a target, not replacing");
}

void CMainWindow::LoadClickConfig(const std::string &config, const std::string &list)
{
    CFlatConfigToScene converter;
    auto lexer = std::make_unique<CLexer>(config);
    auto parser = std::make_unique<CParser>(*lexer, &converter);
    parser->Parse();

    auto *scene = converter.GetScene();
    if (!scene->AutoLayout())
        QMessageBox::information(this, tr("Info"), tr("Automatic layout failed"));

    auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
    view->setScene(scene);
    NavigationPane->SetScene(scene);
    connect(scene,
            &QGraphicsScene::selectionChanged,
            NavigationPane,
            &CNavigationPane::RefreshFromSceneSelection);

    // Keep path table in sync
    ui->pathTable->clear();

    // Validate
    if (!list.empty()) {
        bool validated;
        validated = scene->ValidateAgainstList(list);
        if (!validated)
            ui->statusBar->showMessage(
                "flatconfig and list mismatch! Displayed result may be incorrect.");
    }
}

void CMainWindow::LoadPathHistogram(const std::string &pathHistogram)
{
    auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
    auto *scene = dynamic_cast<CClickGraphScene *>(view->scene());
    if (!scene) {
        ui->statusBar->showMessage("No click config loaded, cannot proceed.");
        return;
    }

    std::stringstream ifs(pathHistogram);

    std::string next;
    std::vector<uint32_t> pathElems;
    while (ifs >> next) {
        pathElems.clear();
        while (ifs) {
            if (next == ":") {
                // We have reached the end of the line
                size_t count;
                ifs >> count;
                if (ifs) {
                    scene->AddPathWithCount(pathElems, count);
                }
                break;
            }
            //Otherwise next should be a number
            pathElems.push_back(std::stoul(next));
            ifs >> next;
        }
    }

    int row = 0;
    ui->pathTable->clear();
    for (const auto &path : scene->GetPaths()) {
        ui->pathTable->insertRow(ui->pathTable->rowCount());

        QString text = path.ToString(scene);
        auto *item = new QTableWidgetItem(text);
        ui->pathTable->setItem(row, 0, item);

        QString count = QString::number(path.PacketCount);
        if (path.bContainsCycle)
            count += "*";
        item = new QTableWidgetItem(count);
        ui->pathTable->setItem(row, 1, item);
        row++;
    }
}

void CMainWindow::on_actionOpen_triggered()
{
    QString fileName
        = QFileDialog::getOpenFileName(this,
                                       tr("Open Click Log"),
                                       "",
                                       tr("Click flat config (flatconfig);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    auto byteArray = file.readAll();
    auto sourceStr = byteArray.toStdString();
    std::string listStr;

    qDebug("Preparing to process file %s", fileName.toUtf8().data());
    if (fileName.endsWith("flatconfig")) {
        auto listPath = fileName.left(fileName.length() - 10) + "list";
        if (QFile::exists(listPath))
            listStr = QFile(listPath).readAll().toStdString();
    }
    LoadClickConfig(sourceStr, listStr);
}

void CMainWindow::on_actionLoadPathHistogram_triggered()
{
    QString fileName
        = QFileDialog::getOpenFileName(this,
                                       tr("Open Click Log"),
                                       "",
                                       tr("Path Histogram (path_histogram);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    auto byteArray = file.readAll();
    auto sourceStr = byteArray.toStdString();
    LoadPathHistogram(sourceStr);
}

void CMainWindow::on_actionShowAll_triggered()
{
    auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
    auto *scene = dynamic_cast<CClickGraphScene *>(view->scene());
    if (!scene) {
        ui->statusBar->showMessage("Nothing to show.");
        return;
    }
    scene->ShowAll();
}

void CMainWindow::on_actionShowSelectionOnly_triggered()
{
    auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
    auto *scene = dynamic_cast<CClickGraphScene *>(view->scene());
    if (!scene) {
        ui->statusBar->showMessage("Nothing to show.");
        return;
    }
    scene->ShowSelectionOnly();
}

void CMainWindow::on_pathTable_itemDoubleClicked(QTableWidgetItem *item)
{
    auto *view = findChild<CGraphicsView *>(OBJ_GRAPHICS_VIEW);
    auto *scene = dynamic_cast<CClickGraphScene *>(view->scene());
    if (!scene) {
        ui->statusBar->showMessage("No click config loaded, cannot proceed.");
        return;
    }

    auto pathIndex = item->row();
    scene->SelectPath(pathIndex);

    //Center on the first element in the path
    if (scene->GetPaths()[pathIndex].Elements.empty())
        return;
    auto *elem = scene->GetElement(scene->GetPaths()[pathIndex].Elements[0]);
    auto pos = elem->pos();
    view->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                      pos.y() + elem->boundingRect().height() / 2);
}

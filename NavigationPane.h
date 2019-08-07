#pragma once
#include "ClickGraphScene.h"
#include "GraphicsView.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QWidget>

class CNavigationPane : public QWidget
{
    Q_OBJECT
public:
    explicit CNavigationPane(QWidget *parent = nullptr);

    void SetScene(CClickGraphScene *scene);
    void SetView(CGraphicsView *view) { View = view; }

    void ClearDisplay();
    void DisplayElement(CElementUI *element);

signals:

public slots:
    void RefreshFromSceneSelection();
    void UpdatePanToListFromPath(uint32_t pathIndex);

private:
    QFormLayout *Layout;
    QLineEdit *EIndexEdit;
    QLineEdit *NameEdit;
    QListWidget *InputList;
    QListWidget *OutputList;
    QTextEdit *ConfigEdit;
    QListWidget *PanToEdit;

    CClickGraphScene *Scene = nullptr;
    CGraphicsView *View = nullptr;
};

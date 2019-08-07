#include "NavigationPane.h"
#include "ElementUI.h"

#include <QCompleter>

#include <sstream>

CNavigationPane::CNavigationPane(QWidget *parent)
    : QWidget(parent)
{
    Layout = new QFormLayout(this);
    Layout->addRow(tr("EIndex"), EIndexEdit = new QLineEdit());
    Layout->addRow(tr("Name"), NameEdit = new QLineEdit());
    Layout->addRow(tr("Inputs"), InputList = new QListWidget());
    Layout->addRow(tr("Outputs"), OutputList = new QListWidget());
    Layout->addRow(tr("Config"), ConfigEdit = new QTextEdit());
    Layout->addRow(tr("Pan To"), PanToEdit = new QListWidget());

    NameEdit->setMinimumWidth(200);

    connect(InputList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (!Scene)
            return;

        auto targetIndex = item->data(Qt::UserRole).toUInt();
        auto *elem = Scene->GetElement(targetIndex);
        auto pos = elem->pos();
        View->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                          pos.y() + elem->boundingRect().height() / 2);

        Scene->clearSelection();
        elem->setSelected(true);
    });

    connect(OutputList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        if (!Scene)
            return;

        auto targetIndex = item->data(Qt::UserRole).toUInt();
        auto *elem = Scene->GetElement(targetIndex);
        auto pos = elem->pos();
        View->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                          pos.y() + elem->boundingRect().height() / 2);

        Scene->clearSelection();
        elem->setSelected(true);
    });

    connect(EIndexEdit, &QLineEdit::editingFinished, [this]() {
        if (!Scene)
            return;
        bool ok;
        auto *elem = Scene->GetElement(EIndexEdit->text().toInt(&ok));
        if (elem && ok && !elem->isSelected()) {
            auto pos = elem->pos();
            View->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                              pos.y() + elem->boundingRect().height() / 2);

            Scene->clearSelection();
            elem->setSelected(true);
        }
    });

    connect(NameEdit, &QLineEdit::editingFinished, [this]() {
        if (!Scene)
            return;
        auto *elem = Scene->GetElement(NameEdit->text().toStdString());
        if (elem && !elem->isSelected()) {
            auto pos = elem->pos();
            View->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                              pos.y() + elem->boundingRect().height() / 2);

            Scene->clearSelection();
            elem->setSelected(true);
        }
    });

    connect(PanToEdit,
            &QListWidget::currentItemChanged,
            [this](QListWidgetItem *item, QListWidgetItem *prev) {
                if (!Scene || !item)
                    return;

                auto targetIndex = item->data(Qt::UserRole).toUInt();
                auto *elem = Scene->GetElement(targetIndex);
                auto pos = elem->pos();
                View->PanToSmooth(pos.x() + elem->boundingRect().width() / 2,
                                  pos.y() + elem->boundingRect().height() / 2);

                DisplayElement(elem);
            });
}

void CNavigationPane::SetScene(CClickGraphScene *scene)
{
    Scene = scene;
    if (Scene) {
        auto n = Scene->CountElements();
        QStringList elemNameList;
        for (decltype(n) i = 0; i < n; i++) {
            auto *e = Scene->GetElement(i);
            elemNameList << QString::fromStdString(e->GetName());
        }
        QCompleter *completer = new QCompleter(elemNameList, NameEdit);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        NameEdit->setCompleter(completer);

        connect(Scene,
                &CClickGraphScene::PathSelectionChanged,
                this,
                &CNavigationPane::UpdatePanToListFromPath);
    }
}

void CNavigationPane::ClearDisplay()
{
    PanToEdit->clear();
    EIndexEdit->setText("");
    NameEdit->setText("");
    InputList->clear();
    OutputList->clear();
    ConfigEdit->clear();
}

void CNavigationPane::DisplayElement(CElementUI *element)
{
    EIndexEdit->setText(QString::number(element->GetEIndex()));
    NameEdit->setText(QString::fromStdString(element->GetName()));

    InputList->clear();
    for (const auto &conn : element->GetConnections(0)) {
        auto *other = conn.second->GetFromNode();
        std::stringstream ss;
        ss << conn.first << ": ";
        ss << other->GetName() << " (" << other->GetEIndex() << ")";
        auto *item = new QListWidgetItem(QString::fromStdString(ss.str()), InputList);
        item->setData(Qt::UserRole, other->GetEIndex());
    }

    OutputList->clear();
    for (const auto &conn : element->GetConnections(1)) {
        auto *toElem = conn.second->GetToNode();
        std::stringstream ss;
        ss << conn.first << ": ";
        ss << toElem->GetName() << " (" << toElem->GetEIndex() << ")";
        auto *item = new QListWidgetItem(QString::fromStdString(ss.str()), OutputList);
        item->setData(Qt::UserRole, toElem->GetEIndex());
    }

    ConfigEdit->clear();
    ConfigEdit->setText(element->GetClassName() + element->GetConfigStr());
}

void CNavigationPane::RefreshFromSceneSelection()
{
    if (!Scene)
        return;
    ClearDisplay();

    auto sel = Scene->selectedItems();
    if (sel.empty())
        return;

    auto *element = qgraphicsitem_cast<CElementUI *>(sel.front());
    if (!element)
        return;
    DisplayElement(element);
}

void CNavigationPane::UpdatePanToListFromPath(uint32_t pathIndex)
{
    PanToEdit->clear();
    for (uint32_t eindex : Scene->GetPaths()[pathIndex].Elements) {
        auto *element = Scene->GetElement(eindex);
        auto *item = new QListWidgetItem(QString::fromStdString(element->GetName()), PanToEdit);
        item->setData(Qt::UserRole, element->GetEIndex());
    }
}

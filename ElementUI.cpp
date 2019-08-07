#include "ElementUI.h"
#include "ConnectionUI.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include <algorithm>

CElementUI::CElementUI(uint32_t eindex,
                       const std::string &name,
                       const std::string &className,
                       const std::string &config,
                       QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , EIndex(eindex)
    , Name(QString::fromStdString(name))
    , ClassName(QString::fromStdString(className))
    , ConfigStr(QString::fromStdString(config))
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setCacheMode(QGraphicsItem::ItemCoordinateCache);
    setFlag(ItemSendsGeometryChanges); //Enable notify position changes
}

CElementUI::~CElementUI() {}

QRectF CElementUI::boundingRect() const
{
    qreal padding = 6.0;
    qreal ypadding = 2.0;
    return QRectF(-padding, -ypadding, 100 + 2 * padding, CardHeight + 2.0 * ypadding);
}

void CElementUI::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    qreal adj = 1.0;
    QRectF rectangle(0, 0, 100, CardHeight);
    QRectF rectangleAdj(0 + adj, 0 + adj, 100 - adj, CardHeight - adj);

    QColor frame(20, 21, 21);
    QColor innerFrame(200, 200, 200, 127);
    QColor innerFrameSelected(200, 80, 80, 200);
    QColor body(67, 67, 87, 127);
    QColor bright(235, 235, 235);

    // Calc color from group
    auto firstSlash = GetName().find_first_of('/');
    if (firstSlash != std::string::npos) {
        auto groupName = GetName().substr(0, firstSlash);
        auto h = std::hash<std::string>()(groupName);
        body.setRed(h & 0xFF);
        body.setGreen((h & 0xFF00) >> 8);
        body.setBlue((h & 0xFF0000) >> 16);
    }

    // Draw card
    painter->fillRect(rectangleAdj, body);
    painter->setPen(frame);
    painter->drawRoundedRect(rectangle, 5, 5);
    painter->setPen(innerFrame);
    if (State == EState::Selected)
        painter->setPen(innerFrameSelected);
    painter->drawRoundedRect(rectangleAdj, 4, 4);

    // Draw name and class
    painter->setPen(bright);
    painter->drawText(rectangleAdj, Qt::AlignCenter, Name + "\n" + ClassName);

    // Draw ports
    painter->setPen(innerFrame);
    painter->setBrush(QBrush(QColor(127, 127, 127)));
    for (int inout = 0; inout <= 1; inout++) {
        for (uint32_t i = 0; i <= MaxIndexSeen[inout]; i++) {
            painter->drawEllipse(GetPortPosition(inout, i), 5, 5);
        }
    }
}

QPointF CElementUI::GetPortPosition(int inout, uint32_t index)
{
    const qreal inx = 0.0;
    const qreal outx_offset = 100.0;
    const qreal header = 10.0;
    const qreal interval = 20.0;
    return QPointF(inx + outx_offset * inout, header + interval * index);
}

void CElementUI::AddConnection(uint32_t inOrOut, uint32_t index, CConnectionUI *conn)
{
    if (inOrOut == 0) {
        InputPorts.emplace(index, conn);
    } else {
        OutputPorts.emplace(index, conn);
    }
    UpdateMaxIndexSeen(inOrOut, index);
    update();
}

void CElementUI::RemoveConnection(uint32_t inOrOut, uint32_t index, CConnectionUI *conn)
{
    if (inOrOut == 0) {
        for (auto iter = InputPorts.begin(); iter != InputPorts.end(); ++iter) {
            if (iter->first == index && iter->second == conn) {
                InputPorts.erase(iter);
                break;
            }
        }
    } else {
        for (auto iter = OutputPorts.begin(); iter != OutputPorts.end(); ++iter) {
            if (iter->first == index && iter->second == conn) {
                OutputPorts.erase(iter);
                break;
            }
        }
    }
    UpdateMaxIndexSeen(inOrOut, index);
    update();
}

void CElementUI::SetState(CElementUI::EState value)
{
    //State machine
    switch (State) {
    case EState::Normal:
        if (value == EState::Selected) {
            for (const auto &pair : InputPorts)
                pair.second->HighlightAsInput(1);
            for (const auto &pair : OutputPorts)
                pair.second->HighlightAsOutput(1);

            State = value;
            update();
        }
        break;
    case EState::Selected:
        if (value == EState::Normal) {
            for (const auto &pair : InputPorts)
                pair.second->HighlightAsInput(-1);
            for (const auto &pair : OutputPorts)
                pair.second->HighlightAsOutput(-1);

            State = value;
            update();
        }
        break;
    }
}

QVariant CElementUI::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        for (const auto &pair : InputPorts)
            pair.second->adjust();
        for (const auto &pair : OutputPorts)
            pair.second->adjust();
    } else if (change == ItemSelectedHasChanged) {
        if (isSelected())
            SetState(EState::Selected);
        else
            SetState(EState::Normal);
    } else if (change == ItemVisibleHasChanged) {
        if (value.toBool()) {
            // Changed into visible, show edges only if both ends are visible
            for (const auto &pair : InputPorts)
                if (pair.second->GetFromNode()->isVisible())
                    pair.second->setVisible(true);
            for (const auto &pair : OutputPorts)
                if (pair.second->GetToNode()->isVisible())
                    pair.second->setVisible(true);
        } else {
            // Just changed into invisible, hide all edges
            for (const auto &pair : InputPorts)
                pair.second->setVisible(false);
            for (const auto &pair : OutputPorts)
                pair.second->setVisible(false);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void CElementUI::UpdateMaxIndexSeen(uint32_t inout, uint32_t index)
{
    MaxIndexSeen[inout] = std::max(MaxIndexSeen[inout], index);
    uint32_t m = std::max(MaxIndexSeen[0], MaxIndexSeen[1]);
    CardHeight = std::max(GetPortPosition(0, m).y() + 15.0, 30.0);
}

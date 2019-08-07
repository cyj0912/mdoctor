#include "ConnectionUI.h"

#include <QPen>

CConnectionUI::CConnectionUI(CElementUI *from, CElementUI *to, uint32_t srcPort, uint32_t dstPort)
    : QGraphicsPathItem(nullptr)
    , FromNode(from)
    , ToNode(to)
    , SrcPort(srcPort)
    , DstPort(dstPort)
{
    FromNode->AddConnection(1, srcPort, this);
    ToNode->AddConnection(0, dstPort, this);
}

void CConnectionUI::adjust()
{
    if (!FromNode || !ToNode)
        return;

    auto fromPos = mapFromItem(FromNode, FromNode->GetPortPosition(1, SrcPort));
    auto toPos = mapFromItem(ToNode, ToNode->GetPortPosition(0, DstPort));
    QLineF line(fromPos, toPos);

    QPainterPath path;
    path.moveTo(fromPos);
    path.lineTo(toPos);
    setPath(path);

    UpdateColor();
}

void CConnectionUI::UpdateColor()
{
    if (InputHighlight + OutputHighlight == 0) {
        QColor edgeDefault(189, 189, 189);
        QPen edgePen(edgeDefault);
        edgePen.setWidthF(1.5);
        setPen(edgePen);
        return;
    }

    auto mul = [](const QColor &lhs, const QColor &rhs) {
        QColor result;
        result.setRedF(lhs.redF() * rhs.redF());
        result.setGreenF(lhs.greenF() * rhs.greenF());
        result.setBlueF(lhs.blueF() * rhs.blueF());
        return result;
    };

    QColor c1(244, 157, 66);
    QColor c2(65, 130, 244);
    QColor c(255, 255, 255);
    for (int i = 0; i < InputHighlight; i++)
        c = mul(c, c1);
    for (int i = 0; i < OutputHighlight; i++)
        c = mul(c, c2);

    QPen edgePen(c);
    edgePen.setWidthF(2.5);
    setPen(edgePen);
}

QVariant CConnectionUI::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSceneHasChanged && scene())
        adjust();
    return QGraphicsPathItem::itemChange(change, value);
}

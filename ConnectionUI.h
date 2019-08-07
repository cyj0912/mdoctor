#pragma once
#include "ElementUI.h"
#include <QGraphicsPathItem>

class CConnectionUI : public QGraphicsPathItem
{
public:
    CConnectionUI(CElementUI *from, CElementUI *to, uint32_t srcPort, uint32_t dstPort);

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    void adjust();
    void UpdateColor();

    void HighlightAsInput(int weight)
    {
        InputHighlight += weight;
        UpdateColor();
    }

    void HighlightAsOutput(int weight)
    {
        OutputHighlight += weight;
        UpdateColor();
    }

    CElementUI *GetFromNode() const { return FromNode; }
    CElementUI *GetToNode() const { return ToNode; }
    uint32_t GetSrcPort() const { return SrcPort; }
    uint32_t GetDstPort() const { return DstPort; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    CElementUI *const FromNode;
    CElementUI *const ToNode;
    const uint32_t SrcPort;
    const uint32_t DstPort;

    int InputHighlight = 0;
    int OutputHighlight = 0;
};

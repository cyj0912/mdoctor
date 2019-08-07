#pragma once
#include "ConnectionUI.h"
#include "ElementUI.h"

#include <unordered_map>
#include <QGraphicsScene>

class CClickGraphScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CClickGraphScene(QObject *parent = nullptr);

    CElementUI *GetElement(uint32_t eindex) const;
    CElementUI *GetElement(const std::string &name) const;
    uint32_t CountElements() const
    {
        assert(ElementsByIndex.size() < UINT32_MAX);
        return ElementsByIndex.size();
    }

    void AddElement(uint32_t eindex,
                    const std::string &name,
                    const std::string &classname,
                    const std::string &configstr);
    void AddConnection(uint32_t srcElement, uint32_t dstElement, uint32_t srcPort, uint32_t dstPort);
    void AddConnection(const std::string &srcElement,
                       const std::string &dstElement,
                       uint32_t srcPort,
                       uint32_t dstPort);

    bool AutoLayout();

    bool ValidateAgainstList(const std::string &list);

    void ShowAll();
    void ShowSelectionOnly();

    //Path stats support
    struct CPath
    {
        size_t Index;
        std::vector<uint32_t> Elements;
        size_t PacketCount;
        bool bContainsCycle;

        QString ToString(CClickGraphScene *scene) const;
    };

    std::vector<uint32_t> CompletePath(const std::vector<uint32_t> &elems);
    void AddPathWithCount(const std::vector<uint32_t> &elems, size_t count);
    void SelectPath(uint32_t pathIndex);
    const std::vector<CPath> &GetPaths() const { return Paths; }

signals:
    void PathSelectionChanged(uint32_t pathIndex);

private:
    std::unordered_map<uint32_t, CElementUI *> ElementsByIndex;
    std::unordered_map<std::string, CElementUI *> ElementsByName;

    //Path stats support
    std::vector<CPath> Paths;
};

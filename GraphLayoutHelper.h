#pragma once

#include <QPointF>

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class CGraphLayoutHelper
{
public:
    typedef std::pair<uint32_t, uint32_t> CPortPair;

    struct CNode
    {
        uint32_t Index;
        std::string Name;
        std::string GroupName;
    };

    CGraphLayoutHelper();

    void AddNode(const CNode &node);
    void AddConnection(uint32_t src, uint32_t srcPort, uint32_t dst, uint32_t dstPort);

    bool ValidateNodeIndices() const;

    std::set<uint32_t> FilterNodes(const std::string &groupName);
    std::map<uint32_t, QPointF> LayoutGroup(const std::set<uint32_t> &group);

    std::map<uint32_t, QPointF> LayoutAll();

private:
    std::vector<CNode> Nodes;
    std::unordered_set<std::string> GroupNames;
    std::map<uint32_t, std::map<uint32_t, CPortPair>> AdjList;
};

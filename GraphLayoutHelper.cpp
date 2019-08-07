#include "GraphLayoutHelper.h"

#include <QTemporaryFile>
#include <QTextStream>

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <random>

CGraphLayoutHelper::CGraphLayoutHelper() {}

void CGraphLayoutHelper::AddNode(const CGraphLayoutHelper::CNode &node)
{
    if (node.Index >= Nodes.size())
        Nodes.resize(node.Index + 1);
    Nodes[node.Index] = node;
    auto firstSlash = Nodes[node.Index].Name.find_first_of('/');
    auto groupName = Nodes[node.Index].Name.substr(0, firstSlash);
    Nodes[node.Index].GroupName = groupName;
    GroupNames.insert(groupName);
}

void CGraphLayoutHelper::AddConnection(uint32_t src,
                                       uint32_t srcPort,
                                       uint32_t dst,
                                       uint32_t dstPort)
{
    AdjList[src][dst] = CPortPair{srcPort, dstPort};
}

std::set<uint32_t> CGraphLayoutHelper::FilterNodes(const std::string &groupName)
{
    std::set<uint32_t> result;
    for (uint32_t i = 0; i < Nodes.size(); i++) {
        if (Nodes[i].GroupName == groupName)
            result.insert(i);
    }
    return result;
}

static std::map<uint32_t, QPointF> RunGraphViz(
    const std::map<uint32_t, std::map<uint32_t, CGraphLayoutHelper::CPortPair>> &adjList,
    const std::set<uint32_t> &group)
{
    std::map<uint32_t, QPointF> result;

    if (group.size() == 1) {
        result[*group.begin()] = QPointF(0, 0);
        return result;
    }

    QTemporaryFile file;
    if (file.open()) {
        QTextStream ofs(&file);
        ofs << "digraph {" << endl;
        for (auto i : group)
            ofs << i << ";" << endl;
        //For every node, we add its outgoing edges
        for (auto i : group) {
            ofs << i << " -> {";
            auto iter = adjList.find(i);
            if (iter != adjList.end()) {
                for (const auto &edge : adjList.at(i)) {
                    auto iter = group.find(edge.first);
                    if (iter == group.end())
                        continue;

                    ofs << edge.first << " ";
                }
            }
            ofs << "};" << endl;
        }
        ofs << "}" << endl;

        //Run graphviz
        QTemporaryFile outputTemp;
        QString outputPath;
        if (outputTemp.open()) {
            outputTemp.close();
            outputPath = outputTemp.fileName();
        } else {
            return {};
        }

        //TODO: wtf
        auto invoke = QString("/Users/Toby.Chen/homebrew/bin/dot -Tplain -Grankdir=LR "
                              "-Gsplines=false -o%1 %2")
                          .arg(outputPath, file.fileName());
        qDebug("%s", invoke.toLocal8Bit().data());
        system(invoke.toLocal8Bit().data());

        std::ifstream outputPlain(outputPath.toStdString(), std::ios::in);
        std::string token;
        while (outputPlain >> token) {
            if (token == "node") {
                uint32_t eindex;
                qreal x, y;
                outputPlain >> eindex >> x >> y;
                result[eindex] = QPointF(x, y);
            } else if (token == "stop")
                break;
        }
        return result;
    }
    return {};
}

std::map<uint32_t, QPointF> CGraphLayoutHelper::LayoutGroup(const std::set<uint32_t> &group)
{
    return RunGraphViz(AdjList, group);
}

std::map<uint32_t, QPointF> CGraphLayoutHelper::LayoutAll()
{
    struct CGroupLayout
    {
        std::map<uint32_t, QPointF> PositionOfNode;
        qreal Width, Height;
        size_t GroupNameHash; //Use hash since name may contain unfavorable characters when appear in graphviz dot files
        QPointF Position;

        bool IsZeroSize() const {
            return Width == 0.0 && Height == 0.0;
        }
    };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution dice(0.8);

    std::unordered_map<size_t, std::string> hashToName;
    std::unordered_map<std::string, CGroupLayout> layoutOfGroup;
    for (const auto& groupName : GroupNames) {
        CGroupLayout groupLayout;
        auto nodeIdVec = FilterNodes(groupName);
        groupLayout.PositionOfNode = LayoutGroup(nodeIdVec);
        // Loop over all positions to determine the group width/height
        qreal minX = 1000000.0, maxX = -1000000.0;
        qreal minY = 1000000.0, maxY = -1000000.0;
        for (const auto& pos : groupLayout.PositionOfNode) {
            qreal x = pos.second.x();
            qreal y = pos.second.y();
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
        groupLayout.Width = maxX - minX;
        groupLayout.Height = maxY - minY;
        groupLayout.GroupNameHash = std::hash<std::string>()(groupName);
        layoutOfGroup[groupName] = groupLayout;
        hashToName[groupLayout.GroupNameHash] = groupName;
    }

    qDebug("Total group count = %lu", layoutOfGroup.size());
    qDebug("Total group hash = %lu", hashToName.size());
    for (const auto& groupName : GroupNames)
        qDebug("%s %lu", groupName.c_str(), layoutOfGroup[groupName].PositionOfNode.size());

    //TODO: wtf
    QFile file("/Users/Toby.Chen/Desktop/layoutOfGroups.txt");
    if (!file.open(QIODevice::ReadWrite))
        return {};
    QTextStream ofs(&file);
    ofs << "digraph {" << endl;
    for (const auto& pair : layoutOfGroup) {
        const auto& groupLayout = pair.second;
        ofs << groupLayout.GroupNameHash;
        if (groupLayout.IsZeroSize()) {
            ofs << ";" << endl;
            continue;
        }
        ofs << " [";
        ofs << "width=" << groupLayout.Width << " ";
        ofs << "height=" << groupLayout.Height << " ";
        ofs << "];" << endl;
    }
    for (const auto& pair1 : AdjList) {
        for (const auto& pair2 : pair1.second) {
            uint32_t u = pair1.first;
            uint32_t v = pair2.first;

            auto uiter = layoutOfGroup.find(Nodes[u].GroupName);
            auto viter = layoutOfGroup.find(Nodes[v].GroupName);
            bool isSameGroup = uiter == viter;

            if (isSameGroup)
                continue;

            if (!dice(gen))
                continue;

            ofs << uiter->second.GroupNameHash << " -> " << viter->second.GroupNameHash;
            ofs << ";" << endl;
        }
    }
    ofs << "}" << endl;

    //TODO: common code
    QTemporaryFile outputTemp;
    QString outputPath;
    if (outputTemp.open()) {
        outputTemp.close();
        outputPath = outputTemp.fileName();
    } else {
        return {};
    }

    auto invoke = QString("/Users/Toby.Chen/homebrew/bin/dot -Tplain -Grankdir=LR "
                          "-Gsplines=false -o%1 %2 -v")
                      .arg(outputPath, file.fileName());
    qDebug("%s", invoke.toLocal8Bit().data());
    system(invoke.toLocal8Bit().data());

    char buffer[1024];
    snprintf(buffer, sizeof (buffer), "cp %s /Users/Toby.Chen/Desktop/layoutResult.txt", outputPath.toUtf8().data());
    system(buffer);

    std::ifstream outputPlain(outputPath.toStdString(), std::ios::in);
    std::string token;
    uint32_t outputCount = 0;
    while (outputPlain >> token) {
        if (token == "node") {
            size_t hash;
            qreal x, y;
            outputPlain >> hash >> x >> y;
            auto groupName = hashToName[hash];
            layoutOfGroup[groupName].Position = QPointF(x, y);
            qDebug("%s: %f %f", groupName.c_str(), x, y);
            outputCount++;
        } else if (token == "stop")
            break;
    }

    std::map<uint32_t, QPointF> result;
    for (const auto& pair : layoutOfGroup) {
        const auto& offset = pair.second.Position;
        for (const auto& element : pair.second.PositionOfNode) {
            result[element.first] = element.second + offset;
        }
    }

    qDebug("outputCount = %u", outputCount);
    qDebug("result.size() = %lu", result.size());
    return result;
}

bool CGraphLayoutHelper::ValidateNodeIndices() const
{
    for (uint32_t i = 0; i < Nodes.size(); i++)
        if (Nodes[i].Index != i)
            return false;
    return true;
}

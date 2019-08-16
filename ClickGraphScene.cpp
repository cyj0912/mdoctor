#include "ClickGraphScene.h"
#include "GraphLayoutHelper.h"

#include <QDialog>
#include <QFile>
#include <QFuture>
#include <QTemporaryFile>
#include <QTextStream>
#include <QtConcurrent>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <queue>

CClickGraphScene::CClickGraphScene(QObject *parent)
    : QGraphicsScene(parent)
{}

CElementUI *CClickGraphScene::GetElement(uint32_t eindex) const
{
    auto iter = ElementsByIndex.find(eindex);
    if (iter == ElementsByIndex.end())
        return nullptr;
    return iter->second;
}

CElementUI *CClickGraphScene::GetElement(const std::string &name) const
{
    auto iter = ElementsByName.find(name);
    if (iter == ElementsByName.end())
        return nullptr;
    return iter->second;
}

void CClickGraphScene::AddElement(uint32_t eindex,
                                  const std::string &name,
                                  const std::string &classname,
                                  const std::string &configstr)
{
    auto *element = new CElementUI(eindex, name, classname, configstr);
    //Hack for the lack of auto layout so that everything is not overlapping
    element->setX(eindex * 150);
    //element->setY((eindex % 200) * 300);
    addItem(element);

    ElementsByName.insert({name, element});
    ElementsByIndex.insert({eindex, element});
}

void CClickGraphScene::AddConnection(uint32_t srcElement,
                                     uint32_t dstElement,
                                     uint32_t srcPort,
                                     uint32_t dstPort)
{
    auto *src = GetElement(srcElement);
    auto *dst = GetElement(dstElement);
    if (!src || !dst) {
        qDebug("Element %d or %d not found", srcElement, dstElement);
        return;
    }

    auto *conn = new CConnectionUI(src, dst, srcPort, dstPort);
    addItem(conn);
}

void CClickGraphScene::AddConnection(const std::string &srcElement,
                                     const std::string &dstElement,
                                     uint32_t srcPort,
                                     uint32_t dstPort)
{
    auto *src = GetElement(srcElement);
    auto *dst = GetElement(dstElement);
    if (!src || !dst) {
        qDebug("Element %s or %s not found", srcElement.c_str(), dstElement.c_str());
        return;
    }

    auto *conn = new CConnectionUI(src, dst, srcPort, dstPort);
    addItem(conn);
}

bool CClickGraphScene::AutoLayout()
{
    CGraphLayoutHelper helper;
    for (uint32_t i = 0; i < ElementsByIndex.size(); i++) {
        CGraphLayoutHelper::CNode node;
        node.Index = i;
        node.Name = ElementsByIndex[i]->GetName();
        helper.AddNode(node);
    }
    if (!helper.ValidateNodeIndices())
        return false;

    for (uint32_t i = 0; i < ElementsByIndex.size(); i++) {
        for (const auto &conn : ElementsByIndex[i]->GetConnections(1)) {
            helper.AddConnection(i,
                                 conn.first,
                                 conn.second->GetToNode()->GetEIndex(),
                                 conn.second->GetDstPort());
        }
    }

    auto outputPositions = helper.LayoutAll();
    for (const auto &pair : outputPositions) {
        ElementsByIndex[pair.first]->setPos(pair.second.x() * 100.0, pair.second.y() * 100.0);
    }
    return true;
}

bool CClickGraphScene::ValidateAgainstList(const std::string &list)
{
    //TODO dummy
    return true;
}

void CClickGraphScene::ShowAll()
{
    for (const auto &pair : ElementsByName) {
        pair.second->setVisible(true);
    }
}

void CClickGraphScene::ShowSelectionOnly()
{
    for (const auto &pair : ElementsByName) {
        if (pair.second->isSelected())
            pair.second->setVisible(true);
        else
            pair.second->setVisible(false);
    }
}

std::vector<uint32_t> CClickGraphScene::CompletePath(const std::vector<uint32_t> &elems)
{
    if (elems.size() == 0)
        return elems;

    std::vector<uint32_t> result;
    for (size_t i = 0; i < elems.size() - 1; i++) {
        result.push_back(elems[i]);
        auto *u = ElementsByIndex[elems[i]];
        auto *v = ElementsByIndex[elems[i + 1]];
        //Test whether u->v is adjacent
        bool areUVAdjacent = false;
        const auto &edges = u->GetConnections(1);
        for (const auto &pair : edges) {
            if (pair.second->GetToNode() == v) {
                areUVAdjacent = true;
                break;
            }
        }

        if (areUVAdjacent)
            continue;

        if (u == v) {
            if (!u->GetClassName().contains("queue", Qt::CaseInsensitive))
                qDebug("!!!!!! %s repeated and is not a queue !!!!!!", u->GetName().c_str());
            i++;
            continue;
        }

        qDebug("%s ... %s not connected", u->GetName().c_str(), v->GetName().c_str());

        // Try to complete the path if not adjacent
        // Dijkstra that biases towards Bypass and the like
        // Vertices are designated by their eindex
        std::priority_queue<std::pair<uint32_t, uint32_t>,
                            std::vector<std::pair<uint32_t, uint32_t>>,
                            std::greater<std::pair<uint32_t, uint32_t>>>
            pq;
        std::vector<uint32_t> dist(CountElements(), UINT32_MAX);
        std::vector<uint32_t> parent(CountElements(), UINT32_MAX);

        uint32_t srcElem = u->GetEIndex();
        uint32_t dstElem = v->GetEIndex();
        pq.push(std::make_pair(0, srcElem));
        dist[srcElem] = 0;
        parent[srcElem] = srcElem;

        bool dijkstraSuccess = false;
        uint32_t iter = 0;
        while (!pq.empty()) {
            unsigned u = pq.top().second;
            pq.pop();

            iter++;
            if (dstElem == u) {
                // Dst poped, done
                dijkstraSuccess = true;
                break;
            }

            const auto &outConns = ElementsByIndex[u]->GetConnections(1);
            for (const auto &pair : outConns) {
                // Get v and calc edge weight
                uint32_t weight = 10;
                CElementUI *vPtr = pair.second->GetToNode();
                uint32_t v = vPtr->GetEIndex();
                if (vPtr->GetClassName() == "Bypass")
                    weight = 4;

                if (dist[v] > dist[u] + weight) {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    pq.push(std::make_pair(dist[v], v));
                }
            }
        }

        if (dijkstraSuccess) {
            std::vector<uint32_t> path;
            uint32_t curr = dstElem;
            while (parent[curr] != curr) {
                path.push_back(parent[curr]);
                curr = parent[curr];
            }

            qDebug("%s (cost=%u iter=%u)", u->GetName().c_str(), dist[dstElem], iter);
            for (auto iter = path.rbegin() + 1; iter != path.rend(); ++iter) {
                qDebug(" -> %s", ElementsByIndex[*iter]->GetName().c_str());
                result.push_back(*iter);
            }
        }
    }
    result.push_back(elems[elems.size() - 1]);
    return result;
}

void CClickGraphScene::AddPathWithCount(const std::vector<uint32_t> &elems, size_t count)
{
    auto cleanedElems = CompletePath(elems);

    auto index = Paths.size();
    auto &path = Paths.emplace_back(CPath{index, cleanedElems, count, false});

    path.bContainsCycle = false;
    if (cleanedElems.size() >= 2) {
        auto e2 = cleanedElems;
        std::sort(e2.begin(), e2.end());
        for (size_t i = 0; i < e2.size() - 1; i++) {
            if (e2[i] == e2[i + 1]) {
                path.bContainsCycle = true;
                break;
            }
        }
    }
}

void CClickGraphScene::SelectPath(uint32_t pathIndex)
{
    clearSelection();
    for (uint32_t eindex : Paths[pathIndex].Elements) {
        ElementsByIndex[eindex]->setSelected(true);
    }
    emit PathSelectionChanged(Paths[pathIndex].Elements);
}

void CClickGraphScene::SelectPath(const std::vector<uint32_t> &elements)
{
    clearSelection();
    for (uint32_t eindex : elements) {
        ElementsByIndex[eindex]->setSelected(true);
    }
    emit PathSelectionChanged(elements);
}

QString CClickGraphScene::CPath::ToString(CClickGraphScene *scene) const
{
    QString result;
    QTextStream ss{&result};
    for (uint32_t eindex : Elements) {
        ss << QString::fromStdString(scene->ElementsByIndex[eindex]->GetName());
        ss << " ";
    }
    return result;
}

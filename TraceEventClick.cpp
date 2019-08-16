#include "TraceEventClick.h"
#include "ClickGraphScene.h"

#include <QColor>

static_assert(sizeof(CEventCommon) == 8, "CEventCommon has inconsistent size");
static_assert(sizeof(CEventDumpHeader) == 48, "CEventDumpHeader has inconsistent size");
static_assert(offsetof(CEventDumpHeader, eindex) == 16, "CEventDumpHeader layout bug");
static_assert(offsetof(CEventDumpHeader, ether_type) == 30, "CEventDumpHeader layout bug");
static_assert(offsetof(CEventDumpHeader, ip_p) == 40, "CEventDumpHeader layout bug");

void CPacketTraceModel::Consume(double timestamp, void *data, size_t len)
{
    const auto *c = reinterpret_cast<CEventCommon *>(data);
    if (c->common_type != 285)
        return;

    qDebug("%f", timestamp);

    const auto *packetHeader = static_cast<const CEventDumpHeader *>(c);

    EventStore.push_back(*packetHeader);
}

int CPacketTraceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return EventStore.size();
}

int CPacketTraceModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 7;
}

QVariant CPacketTraceModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();
    if (role == Qt::DisplayRole) {
        switch (col) {
        case 0:
            return row;
        case 1:
            return EventStore.at(row).skbaddr;
        case 2:
            return QString::fromStdString(
                ClickScene->GetElement(EventStore.at(row).eindex)->GetName());
        case 3:
            return ConvertIPToStringBE(EventStore.at(row).ip_src);
        case 4:
            return ConvertIPToStringBE(EventStore.at(row).ip_dst);
        case 5:
            return ConvertMACToString(EventStore.at(row).ether_shost);
        case 6:
            return ConvertMACToString(EventStore.at(row).ether_dhost);
        }
    }
    if (role == Qt::BackgroundRole) {
        auto skb = EventStore.at(row).skbaddr;
        QColor c;
        //c.setHsl(skb & 0xff, 120, 50);
        c.setHsl(0, 0, (std::hash<uint64_t>()(skb) & 0xff) / 4);

        c.setRed(c.red() + (skb & 0xff) / 18);
        c.setGreen(c.green() + (skb >> 8 & 0xff) / 18);
        c.setBlue(c.blue() + (skb >> 16 & 0xff) / 18);

        return c;
    }
    return {};
}

QVariant CPacketTraceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("Time");
        case 1:
            return QString("Address");
        case 2:
            return QString("Element");
        case 3:
            return QString("SRC IP");
        case 4:
            return QString("DST IP");
        case 5:
            return QString("SRC MAC");
        case 6:
            return QString("DST MAC");
        }
    }
    return {};
}

std::vector<uint32_t> CPacketTraceModel::GetElementsOnPath(const QModelIndex &index) const
{
    assert(index.isValid());
    int row = index.row();
    assert(row < EventStore.size());

    auto skbAddr = EventStore.at(row).skbaddr;

    int row2 = row;
    while (row >= 0 && EventStore.at(row).skbaddr == skbAddr)
        row--;
    row++;

    while (row2 < EventStore.size() && EventStore.at(row2).skbaddr == skbAddr)
        row2++;
    row2--;

    std::vector<uint32_t> result;
    for (int i = row; i <= row2; i++)
        result.push_back(EventStore.at(i).eindex);
    return result;
}

QString CPacketTraceModel::ConvertIPToStringBE(uint32_t ip)
{
    QString result;
    result.sprintf("%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return result;
}

QString CPacketTraceModel::ConvertMACToString(const uint8_t mac[])
{
    QString result;
    result.sprintf("%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return result;
}

void CPacketTraceSorter::SetFilterString(const QString &filter)
{
    qDebug("%s", filter.toStdString().c_str());
    FilterByCol.clear();
    if (!filter.isEmpty()) {
        decltype(filter.size()) curr = 0;
        while (curr >= 0 && curr < filter.size()) {
            int col = -1;
            int ee = filter.indexOf("==", curr);
            QString fieldName = filter.mid(curr, ee - curr);
            fieldName = fieldName.trimmed();
            if (fieldName == "src_ip")
                col = 3;
            else if (fieldName == "dst_ip")
                col = 4;
            int aa = filter.indexOf("&&", ee + 2);
            QString filterDef = filter.mid(ee + 2, aa - ee - 2);
            filterDef = filterDef.trimmed();
            if (col != -1) {
                FilterByCol[col] = filterDef;
                qDebug("%d: %s", col, filterDef.toStdString().c_str());
            }
            curr = aa == -1 ? -1 : aa + 2;
        }
    }
    invalidateFilter();
}

bool CPacketTraceSorter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    for (const auto &pair : FilterByCol) {
        QModelIndex index = sourceModel()->index(source_row, pair.first, source_parent);
        if (!sourceModel()->data(index).toString().contains(pair.second))
            return false;
    }
    return true;
}

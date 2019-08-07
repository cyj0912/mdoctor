#pragma once
#include "TraceDat.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <map>

class CClickGraphScene;
class CPacketTraceModel;

struct CEventCommon
{
    uint16_t common_type;
    uint8_t common_flags;
    uint8_t common_preempt_count;
    int32_t common_pid;
};

struct CEventDumpHeader : public CEventCommon
{
    uint64_t skbaddr;
    uint16_t eindex;
    uint8_t ether_dhost[6];
    uint8_t ether_shost[6];
    uint16_t ether_type;
    uint32_t ip_src;
    uint32_t ip_dst;
    uint8_t ip_p;
};

class CPacketTraceModel : public QAbstractTableModel, public IRecordConsumer
{
public:
    CPacketTraceModel(const CClickGraphScene *clickScene)
        : ClickScene(clickScene)
    {}

    void Consume(double timestamp, void *data, size_t len) override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    static QString ConvertIPToStringBE(uint32_t ip);
    static QString ConvertMACToString(const uint8_t mac[6]);

private:
    const CClickGraphScene *ClickScene;
    std::vector<CEventDumpHeader> EventStore;
};

class CPacketTraceSorter : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void SetFilterString(const QString &filter);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    std::map<int, QString> FilterByCol;
};

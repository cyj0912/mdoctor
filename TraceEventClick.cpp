#include "TraceEventClick.h"

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

static_assert(sizeof(CEventCommon) == 8, "CEventCommon has inconsistent size");
static_assert(sizeof(CEventDumpHeader) == 48, "CEventDumpHeader has inconsistent size");
static_assert(offsetof(CEventDumpHeader, eindex) == 16, "CEventDumpHeader layout bug");
static_assert(offsetof(CEventDumpHeader, ether_type) == 30, "CEventDumpHeader layout bug");
static_assert(offsetof(CEventDumpHeader, ip_p) == 40, "CEventDumpHeader layout bug");

CTraceEventClick::CTraceEventClick() {}

void CTraceEventClick::Consume(double timestamp, void *data, size_t len) const
{
    const auto *c = reinterpret_cast<CEventCommon *>(data);
    if (c->common_type != 285)
        return;
    //qDebug("Event id=%hu", c->common_type);
}

#pragma once
#include "TraceDat.h"

class CTraceEventClick : public IRecordConsumer
{
public:
    CTraceEventClick();

    void Consume(double timestamp, void *data, size_t len) const override;
};

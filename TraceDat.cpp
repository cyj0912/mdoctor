#include "TraceDat.h"

#include <QByteArray>

#include <chrono>

#define CHECK(cond) \
    if (cond) \
        ; \
    else \
        return false;

CTraceDat::CTraceDat(const QString &fileName)
    : File(fileName)
{
    if (!File.open(QIODevice::ReadOnly))
        return;

    if (!CheckHeaderMagic())
        return;
    if (!ParseInitialInfo())
        return;
    if (!ParseEventFormats())
        return;
    if (!ParseKAllSyms())
        return;
    if (!ParseTracePrintkFormats())
        return;
    if (!ParseProcessInformation())
        return;
    if (!ParseRestOfHeader())
        return;

    if (RecType != ERecordType::Flyrecord)
        return;

    bIsLoaded = true;
}

void CTraceDat::ProcessRecords(IRecordConsumer &consumer)
{
    qDebug("======CTraceDat::ProcessRecords======");
    const uint64_t pageSize = 4096; // hard coded
    auto buf = std::make_unique<char[]>(pageSize);
    for (const auto &pair : CPUData) {
        const auto &offset = pair.first;
        const auto &size = pair.second;
        for (uint64_t i = 0; i < size; i += pageSize) {
            bool ok = File.seek(offset + i);
            if (!ok)
                return;
            auto readSize = File.read(buf.get(), pageSize);
            if (readSize != pageSize)
                return;

            auto *pTimestamp = reinterpret_cast<uint64_t *>(&buf[0]);
            auto *pPageLen = reinterpret_cast<uint64_t *>(&buf[8]);
            uint64_t pageTs = *pTimestamp;

            qDebug("%llu %llu", *pTimestamp, *pPageLen);
            uint64_t recordOff = 0;
            while (recordOff < *pPageLen) {
                enum {
                    TYPE_PADDING = 29,
                    TYPE_TIME_EXTEND = 30,
                    TYPE_TIME_STAMP = 31,
                }; // Constants from kernel tools/lib/traceevent/kbuffer.h

                auto *pDeltaAndType = reinterpret_cast<uint32_t *>(&buf[16 + recordOff]);
                uint32_t type = *pDeltaAndType & 0x1f;
                uint32_t delta = *pDeltaAndType >> 5;
                uint64_t recordLen = 0;
                switch (type) {
                case TYPE_PADDING:
                    recordLen = *reinterpret_cast<uint32_t *>(&buf[16 + recordOff + 4]);
                    break;
                case TYPE_TIME_EXTEND:
                    recordLen = 4;
                    break;
                case TYPE_TIME_STAMP:
                    recordLen = 12;
                    break;
                default:
                    recordLen = type * 4;
                    consumer.Consume(pageTs + delta, &buf[16 + recordOff + 4], recordLen);
                    break;
                }
                recordOff += 4; // for DeltaAndType
                recordOff += recordLen;
            }
        }
    }
}

QString CTraceDat::ReadString()
{
    QString str;
    char c;
    while (File.read(&c, 1)) {
        if (c == 0)
            break;
        str.push_back(c);
    }
    return str;
}

bool CTraceDat::ReadInteger(uint16_t &out)
{
    out = 0;
    char c;
    int count = 0;
    while (count < 2 && File.read(&c, 1)) {
        uint64_t ext = static_cast<unsigned char>(c);
        ext <<= 8 * count;
        out |= ext;
        count++;
    }
    if (count >= 2)
        return true;
    return false;
}

bool CTraceDat::ReadInteger(uint32_t &out)
{
    out = 0;
    char c;
    int count = 0;
    while (count < 4 && File.read(&c, 1)) {
        uint64_t ext = static_cast<unsigned char>(c);
        ext <<= 8 * count;
        out |= ext;
        count++;
    }
    if (count >= 4)
        return true;
    return false;
}

bool CTraceDat::ReadInteger(uint64_t &out)
{
    out = 0;
    char c;
    int count = 0;
    while (count < 8 && File.read(&c, 1)) {
        uint64_t ext = static_cast<unsigned char>(c);
        ext <<= 8 * count;
        out |= ext;
        count++;
    }
    if (count >= 8)
        return true;
    return false;
}

bool CTraceDat::CheckHeaderMagic()
{
    //Assuming File is at starting position
    QByteArray data;
    data = File.read(3);
    CHECK(data.length() == 3 && data.at(0) == 0x17 && data.at(1) == 0x08 && data.at(2) == 0x44);
    data = File.read(7);
    CHECK(data.length() == 7 && data == QString("tracing"));
    return true;
}

bool CTraceDat::ParseInitialInfo()
{
    QByteArray data;
    bool ok;
    Version = ReadString().toInt(&ok);
    CHECK(ok);
    data = File.read(1);
    CHECK(data.length() == 1);
    bIsBigEndian = data.at(0);
    data = File.read(1);
    CHECK(data.length() == 1);
    LongSize = data.at(0);
    data = File.read(4);
    CHECK(data.length() == 4);
    return true;
}

bool CTraceDat::ParseEventFormats()
{
    //Event formats are actually not useful for us since we just use one predefined event

    //HEADER INFO FORMAT
    auto str = ReadString();
    CHECK(str == "header_page");
    uint64_t len;
    CHECK(ReadInteger(len));
    auto data = File.read(len);
    CHECK(data.length() == len);

    qDebug("%s", data.toStdString().c_str());

    str = ReadString();
    CHECK(str == "header_event");
    CHECK(ReadInteger(len));
    data = File.read(len);
    CHECK(data.length() == len);

    qDebug("%s", data.toStdString().c_str());

    //FTRACE EVENT FORMATS
    //qDebug("======Now printing Ftrace event formats======");
    uint32_t ftrace_evt_cnt;
    CHECK(ReadInteger(ftrace_evt_cnt));

    for (uint32_t i = 0; i < ftrace_evt_cnt; i++) {
        CHECK(ReadInteger(len));
        data = File.read(len);
        CHECK(data.length() == len);
        //qDebug("%s", data.toStdString().c_str());
    }

    //EVENT FORMATS
    uint32_t system_cnt;
    CHECK(ReadInteger(system_cnt));

    for (uint32_t i = 0; i < system_cnt; i++) {
        str = ReadString();
        uint32_t evt_cnt;
        CHECK(ReadInteger(evt_cnt));

        if (str == "click")
            qDebug("======Now printing events from subsystem %s======", str.toStdString().c_str());

        for (uint32_t j = 0; j < evt_cnt; j++) {
            CHECK(ReadInteger(len));
            data = File.read(len);
            CHECK(data.length() == len);
            if (str == "click")
                qDebug("%s", data.toStdString().c_str());
        }
    }
    return true;
}

bool CTraceDat::ParseKAllSyms()
{
    uint32_t len;
    CHECK(ReadInteger(len));
    auto data = File.read(len);
    CHECK(data.length() == len);
    return true;
}

bool CTraceDat::ParseTracePrintkFormats()
{
    uint32_t len;
    CHECK(ReadInteger(len));
    auto data = File.read(len);
    CHECK(data.length() == len);
    return true;
}

bool CTraceDat::ParseProcessInformation()
{
    uint64_t len;
    CHECK(ReadInteger(len));
    auto data = File.read(len);
    CHECK(data.length() == len);
    return true;
}

bool CTraceDat::ParseRestOfHeader()
{
    uint32_t cpuCount;
    CHECK(ReadInteger(cpuCount));
    qDebug("cpuCount=%u", cpuCount);
    auto str = ReadString();
    if (str == "options  ") {
        while (true) {
            uint16_t opt;
            CHECK(ReadInteger(opt));
            if (opt == 0)
                break;
            uint32_t optLen;
            CHECK(ReadInteger(optLen));
            auto data = File.read(optLen);
            CHECK(data.length() == optLen);
        }
    }

    str = ReadString();

    if (str == "flyrecord")
        RecType = ERecordType::Flyrecord;
    else if (str == "latency  ")
        RecType = ERecordType::Latency;
    else
        return false;

    if (RecType == ERecordType::Flyrecord) {
        for (uint32_t i = 0; i < cpuCount; i++) {
            uint64_t offset, size;
            CHECK(ReadInteger(offset));
            CHECK(ReadInteger(size));
            if (size != 0)
                CPUData.push_back({offset, size});
            qDebug("%llu %llu", offset, size);
        }
    }
    return true;
}

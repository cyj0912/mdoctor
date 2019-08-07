#include "TargetDevice.h"

#include <QProcess>

CTargetDevice::CTargetDevice(QString ip)
    : IPAddress(ip)
{
}

bool CTargetDevice::SanityCheck() const
{
    QProcess proc;
    auto cmd = QString("ssh root@%1 cat /MERAKI_BUILD").arg(IPAddress);
    proc.start(cmd);
    proc.waitForFinished();
    auto outputBytes = proc.readAllStandardOutput();
    qDebug("MERAKI_BUILD: %s", outputBytes.data());
    return outputBytes.length() > 8;
}

std::string CTargetDevice::FetchFlatConfig() const
{
    QProcess proc;
    auto cmd = QString("ssh root@%1 cat /click/flatconfig").arg(IPAddress);
    proc.start(cmd);
    proc.waitForFinished();
    auto outputBytes = proc.readAllStandardOutput();
    return outputBytes.toStdString();
}

void CTargetDevice::EnablePathHistogram() const
{
    QProcess proc;
    auto cmd = QString("ssh root@%1 echo true > click/path_trace_enable").arg(IPAddress);
    proc.start(cmd);
    proc.waitForFinished();
}

void CTargetDevice::DisablePathHistogram() const
{
    QProcess proc;
    auto cmd = QString("ssh root@%1 echo false > click/path_trace_enable").arg(IPAddress);
    proc.start(cmd);
    proc.waitForFinished();
}

void CTargetDevice::ClearPathHistogram() const
{
    //TODO
}

std::string CTargetDevice::FetchPathHistogram() const
{
    QProcess proc;
    auto cmd = QString("ssh root@%1 cat /click/path_histogram").arg(IPAddress);
    proc.start(cmd);
    proc.waitForFinished();
    auto outputBytes = proc.readAllStandardOutput();
    return outputBytes.toStdString();
}

bool CTargetDevice::operator==(const QString &rhs) const
{
    return rhs == IPAddress;
}

//ssh root@172.29.0.153 cat /click/flatconfig

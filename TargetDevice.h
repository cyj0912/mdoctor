#pragma once

#include <QString>

class CTargetDevice
{
public:
    CTargetDevice(QString ip);

    bool IsValid() const { return !IPAddress.isEmpty(); }

    bool SanityCheck() const;
    std::string FetchFlatConfig() const;

    void EnablePathHistogram() const;
    void DisablePathHistogram() const;
    void ClearPathHistogram() const;
    std::string FetchPathHistogram() const;

    bool operator==(const QString &rhs) const;

private:
    //Save as string since we are not using sockets anyway
    QString IPAddress;
};

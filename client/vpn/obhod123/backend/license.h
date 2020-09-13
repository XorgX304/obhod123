#ifndef LICENSE_H
#define LICENSE_H

#include <QObject>

#include "api_http.h"
#include "settings.h"

class License : public QObject
{
    Q_OBJECT
public:
    enum class LicenseMode { TRIAL, FREE, BASIC, PRO, PREMIUM };
    Q_ENUM(LicenseMode)

    static LicenseMode getCurrentLicenseMode();
    static bool isLicenseExpired();

    // Naive implementation
    // Replace with server checks
    static void checkLicenseOnStartup();
    static bool checkLicenseOnConnect();
    static bool isTrialPeriodActive();

    static void initTrialLicense();

private:
    License() = default;
    License(License const &) = delete;
    License& operator= (License const&) = delete;
};

#endif // LICENSE_H

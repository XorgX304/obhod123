#include "license.h"

License::LicenseMode License::getCurrentLicenseMode()
{
    // 5 license modes:
    // -trial
    // -free
    // -basic
    // -pro
    // -premium

    if (Settings::licenseMode() == "basic" ) {
        return LicenseMode::BASIC;
    }
    else if (Settings::licenseMode() == "pro" ) {
        return LicenseMode::PRO;
    }
    else if (Settings::licenseMode() == "premium" ) {
        return LicenseMode::PREMIUM;
    }
    else if (Settings::licenseMode() == "free" ) {
        return LicenseMode::FREE;
    }
    else if (Settings::licenseMode() == "trial" ) {
        return LicenseMode::TRIAL;
    }
    else {
        initTrialLicense();
        return LicenseMode::TRIAL;
    }
}

bool License::isLicenseExpired()
{
    return false;

//    QDateTime currentDateTime = QDateTime::currentDateTime();
//    QDateTime validDateTime = Settings::licenseValidDateTime();

//    if (validDateTime < currentDateTime) return true;
//    else return false;
}

void License::checkLicenseOnStartup()
{
    if (! Settings::hasInstallDateTime()) {
        Settings::setInstallDateTime(QDateTime::currentDateTime());
    }

    if (! Settings::hasUserId() || ! Settings::hasLicenseMode() || ! Settings::hasLicenseValidDateTime() || ! Settings::hasAppMode()) {
        initTrialLicense();
    }
}

bool License::checkLicenseOnConnect()
{
    if (isLicenseExpired()) return false;
    return true;
}

bool License::isTrialPeriodActive()
{
    // check if current license expired less than 7 days ago
    //if (Settings::licenseValidDateTime().addDays(7) > QDateTime::currentDateTime()
    //        && Settings::licenseValidDateTime() < QDateTime::currentDateTime()) return true;

    // check if app installed less than 7 days ago
    if (Settings::appMode() == "trial" && !isLicenseExpired()) return true;
    return false;
}

void License::initTrialLicense()
{
    qDebug() << "License::initTrialLicense";
    Settings::setUserId("free");
    Settings::setAppMode("obhod");
    Settings::setLicenseMode("trial");
    Settings::setLicenseValidDateTime(QDateTime::currentDateTime().addDays(7));
}


﻿
#include <QProcess>
#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#include "tapcontroller_win.h"

#define TAP_EXE_ERROR { \
    qDebug() << "TapController: Can't start tapinstall.exe"; \
    return false; \
    }

#define TAP_NO_MATCHING_DEVICES_ERROR { \
    qDebug() << "TapController: No matching devices found"; \
    return false; \
    }

TapController &TapController::Instance()
{
    static TapController s;
    return s;
}

TapController::TapController()
{
}

bool TapController::checkInstaller()
{
    QProcess tapInstallProc;
    tapInstallProc.start(getTapInstallPath(), QStringList() << "/?");
    if(!tapInstallProc.waitForStarted()) return false;
    else return true;
}

bool TapController::enableTapAdapter(const QString &tapInstanceId)
{
    QProcess tapInstallProc;
    tapInstallProc.start(getTapInstallPath(), QStringList() << "enable" << QString("@") + tapInstanceId);
    if(!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;

    tapInstallProc.waitForFinished();
    QString output = QString(tapInstallProc.readAll());
    if (! output.contains("are enabled")) {
        qDebug() << "TapController: Failed to enable tap device";
        return false;
    }
    if (output.contains("No matching devices ")) TAP_NO_MATCHING_DEVICES_ERROR ;

    qDebug() << "Enabled TAP Instance id:" << tapInstanceId;
    return true;
}

bool TapController::disableTapAdapter(const QString &tapInstanceId)
{
    QProcess tapInstallProc;
    tapInstallProc.start(getTapInstallPath(), QStringList() << "disable" << QString("@") + tapInstanceId);
    if(!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;

    tapInstallProc.waitForFinished();
    QString output = QString(tapInstallProc.readAll());
    if (! output.contains("disabled")) {
        qDebug() << "TapController: Failed to disable tap device";
        return false;
    }
    if (output.contains("No matching devices ")) TAP_NO_MATCHING_DEVICES_ERROR ;

    qDebug() << "Disabled TAP Instance id:" << tapInstanceId;
    return true;
}

QStringList TapController::getTapList()
{
    QProcess tapInstallProc;
    tapInstallProc.start(getTapInstallPath(), QStringList() << "find" << "tap0901" );
    if(!tapInstallProc.waitForStarted()) {
        qDebug() << "TapController: TapController: Can't start tapinstall.exe";
        return QStringList();
    }
    tapInstallProc.waitForFinished();
    QString output = QString( tapInstallProc.readAll() );
    output.replace("\r", "");
    if (output.contains("No matched devices found")) {
        qDebug() << "TapController: No matching device instances found";
        return QStringList();
    }

    QStringList l = output.split("\n", QString::SkipEmptyParts);
    if (l.size() > 0) l.removeLast();

    QStringList tapList;
    for (QString s : l) {
        if (s.contains(" ")) tapList.append(s.split(" ", QString::SkipEmptyParts).first());
        else tapList.append(s);
    }

    return tapList;
}

bool TapController::checkAndSetup()
{
    //////////////////////////////////////////////
    /// Check if OpenVPN executable ready for use
    bool isOpenVpnExeExist = checkOpenVpn();
    if (!isOpenVpnExeExist) {
        qDebug() << "TapController::checkAndSetup :::: openvpn.exe not found";
        return false;
    }

    ////////////////////////////////////////////////
    /// Check if any TAP adapter ready for use
    bool isAnyAvailableTap = false;
    QStringList tapList = getTapList();
    for (const QString &tap : tapList) {
        qDebug() << "TapController: Found TAP device" << tap << ", checking...";
        if (checkDriver(tap)) {
            isAnyAvailableTap = true;
            qDebug() << "TapController: Device" << tap << "is ready for using";
        }
        else
            qDebug() << "TapController: Device" << tap << "is NOT ready for using";
    }

    if (isAnyAvailableTap) {
        qDebug() << "TapController: Check success, found usable TAP adapter";
        return true;
    }
    else qDebug() << "TapController: Check failed, usable TAP adapter NOT found";

    /// Try to setup driver if it's not installed
    qDebug() << "TapController: Installing TAP driver...";
    bool ok = setupDriver();

    if (ok)  qDebug() << "TapController: TAP driver successfully installed";
    else  qDebug() << "TapController: Failed to install TAP driver";

    return ok;
}

bool TapController::checkDriver(const QString& tapInstanceId)
{
    /// Check for driver nodes
    {
        QProcess tapInstallProc;
        tapInstallProc.start(getTapInstallPath(), QStringList() << "drivernodes" << QString("@") + tapInstanceId);
        if (!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;
        tapInstallProc.waitForFinished(1000);

        QString output = QString(tapInstallProc.readAll());
        if (output.contains("No driver nodes found")) {
            qDebug() << "TapController: No driver nodes found";
            return false;
        }
        if (output.contains("No matching devices")) TAP_NO_MATCHING_DEVICES_ERROR ;
    }


    /// Check for files
    {
        QProcess tapInstallProc;
        tapInstallProc.start(getTapInstallPath(), QStringList() << "driverfiles" << QString("@") + tapInstanceId);
        if (!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;
        tapInstallProc.waitForFinished(1000);

        QString output = QString(tapInstallProc.readAll());
        if (output.contains("No driver information")) {
            qDebug() << "TapController: No driver information";
            return false;
        }
        if (output.contains("No matching devices")) TAP_NO_MATCHING_DEVICES_ERROR ;
    }

    /// Check if network adapter enabled
    bool isDisabled = false;
    {
        QProcess tapInstallProc;
        tapInstallProc.start(getTapInstallPath(), QStringList() << "status" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;

        tapInstallProc.waitForFinished();
        QString output = QString(tapInstallProc.readAll());
        output.replace("\r", "");

        if (output.contains("No matching devices ")) TAP_NO_MATCHING_DEVICES_ERROR ;

        if (output.contains("is running")) {
            qDebug() << "TapController: Device" << tapInstanceId << "is active and ready";
            //return true;
        }
        else if (output.contains("is disabled")) isDisabled = true;
        else {
            qDebug() << "TapController: Device" << tapInstanceId << "is in unknown state";
            return false;
        }
    }

    /// Disable adapter if enabled
    if (!isDisabled)  {
        qDebug() << "TapController: Device" << tapInstanceId << "is enabled. Disabling before use...";
        if (!disableTapAdapter(tapInstanceId)) return false;
    }

    /// Enable adapter
    {
        qDebug() << "TapController: Device" << tapInstanceId << "is disabled. Enabling...";
        if (!enableTapAdapter(tapInstanceId)) return false;
    }

    /// Check again
    {
        QProcess tapInstallProc;
        tapInstallProc.start(getTapInstallPath(), QStringList() << "status" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) TAP_EXE_ERROR ;

        tapInstallProc.waitForFinished();
        QString output = QString(tapInstallProc.readAll());

        if (output.contains("is running")) return true;
        else {
            qDebug() << "TapController: tap device final check failed";
            return false;
        }
    }

}

bool TapController::checkOpenVpn()
{
    /// Check openvpn executable

    QProcess openVpnProc;
    openVpnProc.start(getOpenVpnPath(), QStringList() << "--version");
    if (!openVpnProc.waitForStarted()) {
        qDebug() << "TapController: openvpn.exe NOT found";
        return false;
    }
    openVpnProc.waitForFinished(1000);

    QString output = QString(openVpnProc.readAll());
    output.replace("\r", "");
    qDebug() << "TapController: openvpn.exe found, version:" << output;
    return true;
}

QString TapController::getTapInstallPath()
{
#ifdef Q_OS_WIN
    if (QSysInfo::currentCpuArchitecture() == "i386") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x32\\tapinstall.exe";
    }
    else if (QSysInfo::currentCpuArchitecture() == "x86_64") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x64\\tapinstall.exe";
    }
    else return "";
#endif
    return "";
}

QString TapController::getOpenVpnPath()
{
#ifdef Q_OS_WIN
    return qApp->applicationDirPath() + "\\openvpn\\bin\\openvpn.exe";
#endif
    return "";
}

QString TapController::getTapDriverDir()
{
    if (QSysInfo::currentCpuArchitecture() == "i386") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x32\\";
    }
    else if (QSysInfo::currentCpuArchitecture() == "x86_64") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x64\\";
    }
    else return "";
}

bool TapController::removeDriver(const QString& tapInstanceId)
{
    /// remove tap by instance id
    {
        QProcess tapInstallProc;
        tapInstallProc.start(getTapInstallPath(), QStringList() << "remove" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (output.contains("were removed")) {
            qDebug() << "TAP device" << tapInstanceId << "suceccfully removed";
            return true;
        }
        else {
            qDebug() << "Unable to remove TAP device" << tapInstanceId;
            return false;
        }
    }
}

bool TapController::setupDriver()
{
    setupDriverCertificate();

    QStringList tapList = getTapList();
    for (QString tap : tapList) {
        if (! checkDriver(tap)) removeDriver(tap);
    }
    tapList = getTapList();
    if (! tapList.isEmpty()) {
        qDebug() << "TapController: setupDriver :::: Found drivers count" << tapList.size();
        return true;
    }

    /// else try to install driver
    QProcess tapInstallProc;
    tapInstallProc.start(getTapInstallPath(), QStringList() << "install" << getTapDriverDir() + "OemVista.inf" << "tap0901");
    bool ok = tapInstallProc.waitForStarted();
    if (!ok) {
        qDebug() << "TapController: setupDriver failer to start tapInstallProc" << tapInstallProc.errorString();
        return false;
    }


    tapInstallProc.waitForFinished();
    qDebug() << "TapController: setupDriver args" << tapInstallProc.arguments().join(" ");
    qDebug() << "TapController: setupDriver output" << tapInstallProc.readAll();

    /// check again
    tapList = getTapList();
    for (QString tap : tapList) {
        if (! checkDriver(tap)) removeDriver(tap);
    }
    tapList = getTapList();
    if (!tapList.isEmpty()) {
        return true;
    }
    else {
        return false;
    }

}

bool TapController::setupDriverCertificate()
{
    QString cert = getTapDriverDir() + "\\OpenVPN.cer";
    QProcess tapInstallProc;
    tapInstallProc.start("certutil" , QStringList() << "-addstore" << "-f" << "trustedpublisher" << cert);

    tapInstallProc.waitForFinished();

    QString certOutput = QString(tapInstallProc.readAll());
    qDebug() << "TapController: OpenVPN certificate installed:" << certOutput;
    return true;
}


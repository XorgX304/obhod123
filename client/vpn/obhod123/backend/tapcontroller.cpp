#include "tapcontroller.h"

TapController &TapController::Instance()
{
    static TapController s;
    return s;
}

TapController::TapController(QObject *parent) : QObject(parent)
{
    setTapInfo();
}
bool TapController::checkInstaller()
{
    QProcess tapInstallProc;
    tapInstallProc.start(tapInstallDirPath, QStringList() << "/?");
    if(!tapInstallProc.waitForStarted()) return false;
    else return true;
}

QStringList TapController::getTapList()
{
    QProcess tapInstallProc;
    tapInstallProc.start(tapInstallDirPath, QStringList() << "find" << tapName);
    if(!tapInstallProc.waitForStarted()) {
        return QStringList();
    }
    tapInstallProc.waitForFinished();
    QString output = QString( tapInstallProc.readAll() );
    output.replace("\r", "");
    if (output.contains("No matched devices found")) return QStringList();

    QStringList l = output.split("\n", QString::SkipEmptyParts);
    if (l.size() > 0) l.removeLast();

    QStringList tapList;
    for (QString s : l) {
        if (s.contains(" ")) tapList.append(s.split(" ", QString::SkipEmptyParts).first());
        else tapList.append(s);
    }

    return tapList;
}

void TapController::setTapInfo()
{
    if (winhelpIsSystem_x64()) {
        tapDriversDirPath = qApp->applicationDirPath() + "/openvpn/drivers_x64/";
    }
    else {
        tapDriversDirPath = qApp->applicationDirPath() + "/openvpn/drivers_x32/";
    }
    tapName = "tap0901";

    tapInstallDirPath = qApp->applicationDirPath() + "/openvpn/bin/tapinstall.exe";

    qDebug() << "Set TAP driver info:" << tapName << tapDriversDirPath;
}

bool TapController::checkDriver(QString tapInstanceId)
{
    /// check for nodes
    {
        QProcess tapInstallProc;
        tapInstallProc.start(tapInstallDirPath, QStringList() << "drivernodes" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (output.contains("No driver nodes found")) return false;
        if (output.contains("No matching devices ")) return false;
    }

    /// check for files
    {
        QProcess tapInstallProc;
        tapInstallProc.start(tapInstallDirPath, QStringList() << "driverfiles" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (output.contains("No driver information")) return false;
        if (output.contains("No matching devices ")) return false;
    }

    /// check if enabled
    bool isDisabled = false;
    {
        QProcess tapInstallProc;
        tapInstallProc.start(tapInstallDirPath, QStringList() << "status" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (output.contains("No matching devices ")) return false;

        if (output.contains("is running")) return true;
        else if (output.contains("is disabled")) isDisabled = true;
        else return false;
    }

    /// Try to enable
    if (isDisabled) {
        QProcess tapInstallProc;
        tapInstallProc.start(tapInstallDirPath, QStringList() << "enable" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (! output.contains("are enabled")) return false;
        if (output.contains("No matching devices ")) return false;
    }

    /// Check again
    QProcess tapInstallProc;
    tapInstallProc.start(tapInstallDirPath, QStringList() << "status" << QString("@") + tapInstanceId);
    if(!tapInstallProc.waitForStarted()) return false;

    tapInstallProc.waitForFinished();
    QString output = QString( tapInstallProc.readAll() );

    if (output.contains("is running")) return true;
    else return false;
}

bool TapController::setupDriver()
{
    QStringList tapList = getTapList();
    for (QString tap : tapList) {
        if (! checkDriver(tap)) removeDriver(tap);
    }
    tapList = getTapList();
    if (! tapList.isEmpty()) return true;

    /// else try to install driver
    QProcess tapInstallProc;
    tapInstallProc.start(tapInstallDirPath, QStringList() << "install" << tapDriversDirPath + "/OemVista.inf" << tapName);

    qDebug() << "TapController::setupDriver :::: tapInstallProc.start failed" << tapInstallDirPath << tapInstallProc.arguments().join(" ");

    bool ok =  tapInstallProc.waitForStarted(1000);
    if (!ok) {
        qDebug() << "TapController::setupDriver :::: tapInstallProc.start failed" << tapInstallDirPath << tapInstallProc.arguments().join(" ");
    }

    tapInstallProc.waitForFinished();

    /// check again
    tapList = getTapList();
    for (QString tap : tapList) {
        if (! checkDriver(tap)) removeDriver(tap);
    }
    tapList = getTapList();

    if (! tapList.isEmpty()) return true;
    else return false;
}

bool TapController::removeDriver(QString tapInstanceId)
{
    /// remove tap by instance id
    {
        QProcess tapInstallProc;
        tapInstallProc.start(tapInstallDirPath, QStringList() << "remove" << QString("@") + tapInstanceId);
        if(!tapInstallProc.waitForStarted()) return false;

        tapInstallProc.waitForFinished();
        QString output = QString( tapInstallProc.readAll() );
        if (output.contains("were removed")) return true;
        else return false;
    }
}

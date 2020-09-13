

#include "updater.h"
#define Updater_DEBUG 1
//#define QUICK_TEST


Updater &Updater::Instance()
{
    static Updater s;
    return s;
}

QString Updater::getRepoUrl()
{
#ifdef Q_OS_WIN
    return "/updates/windows/repo";
#endif

#ifdef Q_OS_OSX
    return "/updates/osx/repo";
#endif

    return "";
}

bool Updater::checkForUpdates(const QString &server)
{
    // Check server for file Updates.xml
    QString url = "http://" + server + ":8080" + getRepoUrl() + "/Updates.xml";

    QByteArray ba = API_HTTP::Instance().GET_SYNC(url, 1000);
    if (!QString(ba).contains("PackageUpdate")) {
        qDebug() << "Updater :: Updates.xml is not accesible at url" << url;
        return false;
    }

    qDebug() << "Updater :: Updates.xml found, checking for updates";

    QString currentVersion = getCurrentVersion();
    QString newVersion = getNewVersion(QString(ba));

    bool needUpdate = isUpdateAvailable(currentVersion, newVersion);

    qDebug() << "Updater::checkForUpdates :currentVersion, newVersion" << currentVersion << newVersion;
    if (needUpdate) qDebug() << "New version Found, starting update";
    else qDebug() << "New version Not Found";
    return needUpdate;
}


QString Updater::maintenanceToolName()
{
#ifdef Q_OS_WIN
    // On Windows and Linux MaintenanceTool placed in the same directory where client executable
    // Filename must be without spaces (For Windows)!
    return "maintenancetool.exe";

#else //Q_OS_LINUX and Q_OS_OSX
    return "maintenancetool";
#endif

}

QString Updater::pathToMaintenanceTool() const
{
#ifdef Q_OS_OSX
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
            QString("/Library/Application Support/Obhod123/maintenancetool.app/Contents/MacOS/%1").arg(maintenanceToolName());
#else
    return qApp->applicationDirPath() + QDir::separator() + maintenanceToolName();
#endif
}

bool Updater::isUpdateAvailable(const QString &currentVersion, const QString &newVersion)
{
    if (currentVersion.isEmpty() || newVersion.isEmpty()) return false;

    if (newVersion.split(".").size() > currentVersion.split(".").size()) return true;
    if (newVersion.split(".").size() < currentVersion.split(".").size()) return false;

    for (int i = 0; i < newVersion.split(".").size(); ++i) {
        int newVersionDigit = newVersion.split(".").at(i).toInt();
        int currentVersionDigit = currentVersion.split(".").at(i).toInt();

        if (newVersionDigit > currentVersionDigit) return true;
        if (newVersionDigit < currentVersionDigit) return false;
    }
    return false;
}

QString Updater::getComponentsXmlPath()
{
#ifdef Q_OS_WIN
    return qApp->applicationDirPath() + QDir::separator() + "components.xml";
#endif

    return "";
}

QString Updater::getCurrentVersion()
{
    if (getComponentsXmlPath().isEmpty()) return "";

    QFile file(getComponentsXmlPath());
    if (!file.open(QIODevice::ReadOnly)) return "";

    QString xml = QByteArray(file.readAll());

    QRegularExpression re("(<Version>)([\\d\\.]+)(</Version>)");
    QRegularExpressionMatch match = re.match(xml);

    if (match.hasMatch()) {
        return match.captured(2);
    }

    return "";
}

QString Updater::getNewVersion(const QString &xml)
{
    QRegularExpression re("(<Version>)([\\d\\.]+)(</Version>)");
    QRegularExpressionMatch match = re.match(xml);

    if (match.hasMatch()) {
        return match.captured(2);
    }

    return "";
}

//bool Updater::checkForUpdates(const QString &server)
//{
//    qDebug() << "Updater: Started checking, path to MaintenanceTool is: " << pathToMaintenanceTool();

//    QPointer<QProcess> updateCheckProcess = new QProcess;
//    updateCheckProcess->setProcessChannelMode(QProcess::MergedChannels);


//    QStringList params = QStringList()
//            << "--checkupdates"
//            << "--setTempRepository" << "http://" + server + ":8080" + getRepoUrl();
//    updateCheckProcess->start(pathToMaintenanceTool(), params);

//    qDebug().noquote() << "Updater: Starting" << pathToMaintenanceTool() << params.join(" ");


//    bool ok = updateCheckProcess->waitForStarted(1000);
//    if (!ok) {
//        qDebug().noquote() << "Updater: Check for updates: failed to start updater";
//        return false;
//    }

//    ok = updateCheckProcess->waitForFinished(1000);
//    if (!ok) {
//        qDebug().noquote() << "Updater: Check for updates: failed to finish updater";
//        return false;
//    }

//    QString output = QString(updateCheckProcess->readAll());


//    bool haveUpdates = false;
//    if (output.contains("There are no updates available")) {
//        haveUpdates = false;
//    }
//    else {
//        QRegularExpression re("version=\"[\\d,\\.]+\"");
//        QRegularExpressionMatch match = re.match(output);
//        if (match.hasMatch()) {
//            haveUpdates = true;
//            QString matched = match.captured(0);
//            matched.replace("version=", "");
//            matched.replace("\"", "");
//        }
//    }
//    if (updateCheckProcess) updateCheckProcess->deleteLater();

//    qDebug().noquote() << "Updater: Check for updates output: " + QString(haveUpdates ? "\n" : "") << output.trimmed();

//    if (updateCheckProcess) updateCheckProcess->kill();

//    return haveUpdates;
//}

void Updater::updateApp(const QString &server)
{
    qDebug() << "Updater: Update started";

    // New, improved version
    qint64 pid;
    QStringList params = QStringList()
            << "--updater"
            << "--setTempRepository" << "http://" + server + ":8080" + getRepoUrl();

    bool started = QProcess:: startDetached(pathToMaintenanceTool(), params, QString(), &pid);
    qDebug().noquote().nospace() << "Updater: MaintenanceTool with params " << params <<  " started: " << started << ", pid: " << pid;
}

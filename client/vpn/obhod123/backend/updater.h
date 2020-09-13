#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QTimer>
#include <QPointer>
#include <QStandardPaths>
#include <QRegularExpressionMatch>

#include <functional>

#include "api_http.h"


//! The Updater is the class checks for updates and notifies the user by displaying a dialog box.
class Updater : public QObject
{
    Q_OBJECT

public:
    static Updater& Instance();
    static QString getRepoUrl();
    static QString maintenanceToolName();

    bool checkForUpdates(const QString &server);
    void updateApp(const QString &server);

    //! Return executable name (without path) for Maintenance Tool



private:
    // If child process still running and should be killed because timeout
    const int timeoutToForceKillChildProcess = 3000;
    QString pathToMaintenanceTool() const;

    Updater() = default;
    Updater(Updater const &) = delete;
    Updater& operator= (Updater const&) = delete;

    static bool isUpdateAvailable(const QString &currentVersion, const QString &newVersion);

    static QString getComponentsXmlPath();
    static QString getCurrentVersion();
    static QString getNewVersion(const QString &xml);

    //bool checkForUpdates(const QString &server); // not working due bug
};

#endif // UPDATER_H

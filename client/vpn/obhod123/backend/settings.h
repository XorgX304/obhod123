#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDateTime>
#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);

    // check if app started firts time
    static bool notFirtstRun() { return hasInstallDateTime(); }

    // datetime when app was installed (first run)
    static QDateTime installDateTime() { return q_settings().value("installDateTime").toDateTime(); }
    static bool hasInstallDateTime() { return q_settings().contains("installDateTime"); }
    static void setInstallDateTime(const QDateTime &installDateTime) { q_settings().setValue("installDateTime", installDateTime); }

    // license key aka user_id
    static QString userId() { return q_settings().value("userId").toString(); }
    static bool hasUserId() { return q_settings().contains("userId"); }
    static void setUserId(const QString &userId) { q_settings().setValue("userId", userId); }

    // appMode: obhod, pro, premium
    static QString appMode() { return q_settings().value("mode").toString(); }
    static bool hasAppMode() { return q_settings().contains("mode"); }
    static void setAppMode(const QString &appMode) { q_settings().setValue("mode", appMode); }

    // licenseMode: free, trial, basic, pro, premium
    static QString licenseMode() { return q_settings().value("licenseMode").toString(); }
    static bool hasLicenseMode() { return q_settings().contains("licenseMode"); }
    static void setLicenseMode(const QString &licenseMode) { q_settings().setValue("licenseMode", licenseMode); }

    // license valid till: licenseValidDateTime
    static QDateTime licenseValidDateTime() { return q_settings().value("licenseValidDateTime").toDateTime(); }
    static bool hasLicenseValidDateTime() { return q_settings().contains("licenseValidDateTime"); }
    static void setLicenseValidDateTime(const QDateTime &licenseValidDateTime) { q_settings().setValue("licenseValidDateTime", licenseValidDateTime); }

    //////////////////////////////////////////////

    // currentServerId: last store server id
    //static QString currentServerIp() { return q_settings().value("currentServerIp").toString(); }
    static QString currentServerIp() { return "54.38.141.191"; }
    static bool hasCurrentServerIp() { return q_settings().contains("currentServerIp"); }
    static void setCurrentServerIp(const QString &currentServerIp) { q_settings().setValue("currentServerIp", currentServerIp); }

    // autoServer - whether or not select server automatically during connect
    static bool autoServer() { return q_settings().value("autoServer").toBool(); }
    static bool hasAutoServer() { return q_settings().contains("autoServer"); }
    static void setAutoServer(bool autoServer) { q_settings().setValue("autoServer", autoServer); }

    // protocol: udp, l2tp, ikev2...
    static QString protocol() { return q_settings().value("protocol").toString(); }
    static bool hasProtocol() { return q_settings().contains("protocol"); }
    static void setProtocol(const QString &protocol) { q_settings().setValue("protocol", protocol); }

    // connectWhenStarted
    static bool connectWhenStarted() { return q_settings().value("connectWhenStarted").toBool(); }
    static bool hasConnectWhenStarted() { return q_settings().contains("connectWhenStarted"); }
    static void setConnectWhenStarted(bool connectWhenStarted) { q_settings().setValue("connectWhenStarted", connectWhenStarted); }

    // launchOnStartup
    static bool launchOnStartup() { return q_settings().value("launchOnStartup").toBool(); }
    static bool hasLaunchOnStartup() { return q_settings().contains("launchOnStartup"); }
    static void setLaunchOnStartup(bool launchOnStartup) { q_settings().setValue("launchOnStartup", launchOnStartup); }

    // launchOnStartup
    static bool reconnectOnDrop() { return q_settings().value("reconnect").toBool(); }
    static bool hasReconnectOnDrop() { return q_settings().contains("reconnect"); }
    static void setReconnectOnDrop(bool reconnect) { q_settings().setValue("reconnect", reconnect); }

    // networkLock aka killswitch
    static bool networkLock() { return q_settings().value("networkLock").toBool(); }
    static bool hasNetworkLock() { return q_settings().contains("networkLock"); }
    static void setNetworkLock(bool launchOnStartup) { q_settings().setValue("networkLock", launchOnStartup); }

    // mtu size
    static int mtuSize() { return q_settings().value("mtuSize").toInt(); }
    static bool hasMtuSize() { return q_settings().contains("mtuSize"); }
    static void setMtuSize(int mtuSize) { q_settings().setValue("mtuSize", mtuSize); }

    // proxyType
    static QString proxyType() { return q_settings().value("proxyType").toString(); }
    static bool hasProxyType() { return q_settings().contains("proxyType"); }
    static void setProxyType(const QString &proxyType) { q_settings().setValue("proxyType", proxyType); }

    // proxyAddress
    static QString proxyAddress() { return q_settings().value("proxyAddress").toString(); }
    static bool hasProxyAddress() { return q_settings().contains("proxyAddress"); }
    static void setProxyAddress(const QString &proxyAddress) { q_settings().setValue("proxyAddress", proxyAddress); }

    // proxyPort
    static int proxyPort() { return q_settings().value("proxyPort").toInt(); }
    static bool hasProxyPort() { return q_settings().contains("proxyPort"); }
    static void setProxyPort(int proxyPort) { q_settings().setValue("proxyPort", proxyPort); }

    //////////////////////////////////////////////

    // favorites servers: QStringList of servers ids
    static QStringList favoritesServers() { return q_settings().value("favorites").toStringList(); }
    static bool hasFavoritesServers() { return q_settings().contains("favorites"); }
    static void setFavoritesServers(const QStringList &favorites) { q_settings().setValue("favorites", favorites); }


    // list of sites to pass blocking added by user
    static QStringList customSites() { return q_settings().value("customSites").toStringList(); }
    static void setCustomSites(const QStringList &customSites) { q_settings().setValue("customSites", customSites); }

    // list of ips to pass blocking generated from customSites
    static QStringList customIps() { return q_settings().value("customIps").toStringList(); }
    static void setCustomIps(const QStringList &customIps) { q_settings().setValue("customIps", customIps); }

signals:

public slots:

private:
    Settings() = default;
    Settings(Settings const &) = delete;
    Settings& operator= (Settings const&) = delete;

    static QSettings &q_settings();
};

#endif // SETTINGS_H

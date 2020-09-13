#ifndef API_HTTP_H
#define API_HTTP_H


#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QMap>
#include <QStringList>
#include <QSettings>
#include <QPointer>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QDir>
#include <QUrlQuery>

#include <functional>

#include "cryptofuck.h"

/**
 * @brief The API_HTTP class - API to get the initial network configuration and check the updates
 */
class API_HTTP : public QObject
{
    Q_OBJECT
public:
    static API_HTTP& Instance();
    void resetNAM();

    void setTrialUserId() { m_user_id = "free"; }

    enum AuthResult { AUTH_SUCCESS, AUTH_TIMEOUT, AUTH_KEY_NOT_FOUND, AUTH_KEY_EXPIRED };
    Q_ENUM(AuthResult)
    AuthResult requestAuth(const QString& user_id);
    AuthResult requestAuthWorker(const QString& user_id, const QString& serverIp);

    enum ActivationResult { ACTIVATION_SUCCESS, ACTIVATION_TIMEOUT, ACTIVATION_KEY_NOT_FOUND, ACTIVATION_KEY_EXPIRED };
    Q_ENUM(ActivationResult)
    ActivationResult activateNewLicense(const QString& new_user_id, const QString& old_user_id);
    ActivationResult activateNewLicenseWorker(const QString& new_user_id, const QString& old_user_id, const QString& serverIp);

    bool requestServers(const QList<QPair<QString, double> > &serverIds);
    bool requestServersWorker(const QString& serverIp);

    bool requestBlockedIpsWorker(const QString& serverIp);
    bool requestBlockedIps();


    bool loadServersFromDisk();
    bool loadServersFromDisk_text();
    void saveServersToDisk();

    QString serversListPath() { return qApp->applicationDirPath() + QDir::separator() + "servers.dat"; }

    bool initServerListText(const QByteArray& textData);
    bool initServerListRaw(const QByteArray& rawData);
    bool initServerList(const QJsonObject& o);

    void requestDetectedLocation(std::function<void(QJsonObject)> func, int timeout = 10000);
    bool getIgnoreSslErrors() const;
    void setIgnoreSslErrors(bool ignore);

    const QJsonObject authData() const;
    void clearAuthData();
    const QJsonArray getServersInRegion(const QString& region) const;
    const QJsonObject getServerById(const QString& id) const;
    const QStringList getRegions() const;

    const QStringList getBlackIPs() const;

    QString getUserId() const;
    bool getServersListDownloaded() const;
    QJsonArray getServersList() const;

    QNetworkConfigurationManager confManager;

    // Naive implementation!!!
    // Client should generate his own cert, sign by server cert, and send to server
    QString downloadOVPNConfiguration(const QJsonObject &server, const QString &protocol);


    QByteArray GET_SYNC(const QUrl& url, int timeout = 10000);

    // Naive implementation!!!
    // Need to generate one time session key during vpn session configuration
    QByteArray GET_SYNC_ENCRYPTED(const QString& prefix, const QString &command, const QUrlQuery &query = QUrlQuery(), int timeout = 10000);

    QByteArray POST_SYNC(const QUrl& url, const QByteArray& data, int timeout = 30000);
    QByteArray PUT_SYNC(const QUrl& url, const QByteArray& data, int timeout = 30000);
    void GET_ASYNC(const QUrl& url, std::function<void(QByteArray)> func, int timeout = 10000);
signals:
    void serverListChanged();
    void appUpdated();
    void appUpdateFound();

private:
    API_HTTP();
    API_HTTP(API_HTTP const &) = delete;
    API_HTTP& operator= (API_HTTP const&) = delete;



    QString serverPrefix(QString server_ip);

    QPointer<QNetworkAccessManager> manager;

    QJsonObject m_authData;
    QString m_user_id;

    QMap<QString, QJsonArray> regionsMap; // <region, list servers>
    QMap<QString, QJsonObject> serversMap; // <server id, server data>

    bool isServerListDownloaded;
    bool ignoreSslErrors;

    QString maintenancetool();

    QString m_ovpnFile;

    QStringList m_blackIPs;


    QJsonObject serversObject; // full object, serverList in "servers" key - //QJsonArray serversList;
};

#endif // API_HTTP_H

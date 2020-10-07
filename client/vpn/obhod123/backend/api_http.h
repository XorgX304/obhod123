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

    bool requestBlockedIpsWorker(const QString& serverIp);
    bool requestBlockedIps();

    bool getIgnoreSslErrors() const;
    void setIgnoreSslErrors(bool ignore);

    const QJsonObject authData() const;
    void clearAuthData();

    const QStringList getBlackIPs() const;

    bool getServersListDownloaded() const;
    QJsonObject getCurrentServer() const;

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

    bool isServerListDownloaded;
    bool ignoreSslErrors;

    QString maintenancetool();

    QString m_ovpnFile;

    QStringList m_blackIPs;


    QJsonObject serversObject; // full object, serverList in "servers" key - //QJsonArray serversList;
};

#endif // API_HTTP_H

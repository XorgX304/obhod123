#include <QDir>
#include <QUrlQuery>
#include <QTimer>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QProcess>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFuture>

#include "api_http.h"
#include "settings.h"

#define API_HTTP_DEBUG 1
//#undef API_HTTP_DEBUG


API_HTTP::API_HTTP() :
    ignoreSslErrors(true),
    isServerListDownloaded(false)
{
    manager = new QNetworkAccessManager(this);
    manager->setConfiguration(confManager.defaultConfiguration());
}

API_HTTP &API_HTTP::Instance()
{
    static API_HTTP s;
    return s;
}

void API_HTTP::resetNAM()
{
    manager->deleteLater();
    manager = new QNetworkAccessManager(this);
}

bool API_HTTP::requestBlockedIps()
{
    // TODO now save list to disk
    //saveToDisk();

    return false;
}

bool API_HTTP::requestBlockedIpsWorker(const QString &serverIp)
{
    QUrl url = QString("http://%1:8080/updates/ipsc.txt").arg(serverIp);
    QByteArray ba = GET_SYNC(url);
    if (ba.size() > 10) {
        m_blackIPs = QString(ba).split("\n");
        m_blackIPs.prepend("8.8.8.8");
        m_blackIPs.prepend("8.8.8.4");

        return true;
    }
    else {
        return false;
    }
}

QByteArray API_HTTP::GET_SYNC(const QUrl& url, int timeout)
{
    if (!manager) return QByteArray();

    QPointer<QEventLoop> loop = new QEventLoop;
    QPointer<QNetworkReply> reply = manager->get(QNetworkRequest(url));
    connect(manager, &QNetworkAccessManager::finished, [&, reply, loop](QNetworkReply *r) {
        if (r == reply) {
            if (loop) {
                loop->quit();
            }
        }
    });

    if (ignoreSslErrors) {
        connect(reply, &QNetworkReply::sslErrors, [&, reply](const QList<QSslError> &errors){
            reply->ignoreSslErrors(errors);
        });
    }

    QTimer::singleShot(timeout, loop, SLOT(quit()));
    loop->exec();


    QByteArray response;
    if (reply) response = reply->readAll();

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::GET_SYNC " << url.toString() << "\nAPI::GET response size:" << response.size();
#endif

    if (reply) reply->deleteLater();
    if (loop) loop->deleteLater();

    return response;
}

QByteArray API_HTTP::GET_SYNC_ENCRYPTED(const QString &prefix, const QString &command, const QUrlQuery &query, int timeout)
{
    QString encryptedRequest = command + "?" + query.toString(QUrl::FullyDecoded);
    encryptedRequest = QString(encryptByteArray(encryptedRequest.toLatin1()));

    QUrl url(prefix + "/" + encryptedRequest);
    return GET_SYNC(url, timeout);
}

QByteArray API_HTTP::POST_SYNC(const QUrl& url, const QByteArray& data, int timeout)
{
    if (!manager) return QByteArray();

    QPointer<QEventLoop> loop = new QEventLoop;
    QPointer<QNetworkReply> reply = manager->post(QNetworkRequest(url), data);
    connect(manager, &QNetworkAccessManager::finished, [&, reply, loop](QNetworkReply *r) {
        if (r == reply) {
            if (loop) {
                loop->quit();
            }
        }
    });

    if (ignoreSslErrors) {
        connect(reply, &QNetworkReply::sslErrors, [&, reply](const QList<QSslError> &errors){
            reply->ignoreSslErrors(errors);
        });
    }

    QTimer::singleShot(timeout, loop, SLOT(quit()));
    loop->exec();


    QByteArray response;
    if (reply) response = reply->readAll();

    if (reply) reply->deleteLater();
    if (loop) loop->deleteLater();

    return response;
}

void API_HTTP::GET_ASYNC(const QUrl& url, std::function<void(QByteArray)> func, int timeout)
{
    if (!manager) return;

    QPointer<QNetworkReply> reply = manager->get(QNetworkRequest(url));
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, func, reply](QNetworkReply::NetworkError e) {
        Q_UNUSED(e)
#ifdef API_HTTP_DEBUG
        qDebug().noquote() << "API_HTTP::GET_ASYNC ERROR" << e;
#endif
        func(QByteArray());
    });

    connect(reply, &QNetworkReply::finished, [this, func, reply]() {
        QByteArray response;
        if (reply) {
            response = reply->readAll();
            func(response);
            reply->deleteLater();
        }
#ifdef API_HTTP_DEBUG
        qDebug().noquote() << "API_HTTP::GET_ASYNC " << reply->url().toString() << "\nAPI::GET response size:" << response.size();
#endif

    });

    if (ignoreSslErrors) {
        connect(reply, &QNetworkReply::sslErrors, [&, reply](const QList<QSslError> &errors){
            reply->ignoreSslErrors(errors);
        });
    }

    QTimer::singleShot(timeout, [&, reply](){
        if (reply) {

#ifdef API_HTTP_DEBUG
            qDebug() << "API_HTTP::GET_ASYNC Abort";
#endif

            reply->abort(); // finished signal will be emitted
        }
    });
}

QString API_HTTP::serverPrefix(QString server_ip)
{
    return QString("http://%1/%2").arg(server_ip).arg(API_VERSION);
}

bool API_HTTP::getServersListDownloaded() const
{
    return isServerListDownloaded;
}

QJsonObject API_HTTP::getCurrentServer() const
{
    QJsonObject server;
    server.insert("id", "1");
    server.insert("name", "My Server");
    server.insert("ip", Settings::currentServerIp());

    return server;
}

const QStringList API_HTTP::getBlackIPs() const
{
    return m_blackIPs;
}

const QJsonObject API_HTTP::authData() const
{
    return m_authData;
}

void API_HTTP::clearAuthData()
{
    m_authData = QJsonObject();
}

bool API_HTTP::getIgnoreSslErrors() const
{
    return ignoreSslErrors;
}

void API_HTTP::setIgnoreSslErrors(bool ignore)
{
    ignoreSslErrors = ignore;
}

QString API_HTTP::downloadOVPNConfiguration(const QJsonObject &server, const QString& protocol)
{
    QString url;

    if (! (protocol == "udp" || protocol == "tcp" || protocol == "ovpn") ) return "";

    if (server.value("ip").toString().isEmpty()) {
        m_ovpnFile = "";
        return "\n";
    }


//    url = QString("%1/c_getOvpnFile?name=%2&pw=%3")
//            .arg(serverPrefix(server.value("ip").toString()))
//            .arg(m_user_id)
//            .arg(m_password);


    qDebug().noquote() << "Downloading vpn configuration, url: " << url;



    QUrlQuery q;
    q.addQueryItem("user_id", "free");

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::requestAuth:" << q.toString();
#endif

    QString serv_prefix = serverPrefix(server.value("ip").toString());

    QByteArray encrypted_response = GET_SYNC_ENCRYPTED(serv_prefix, "c_getOvpnFile", q, 3000);
    QByteArray decrypted_response = decryptByteArray(encrypted_response);

    if (decrypted_response.isEmpty() && !encrypted_response.isEmpty()) return "\n"; // needed to avoid resetting QNAM

    m_ovpnFile = QString(decrypted_response);


    m_ovpnFile.replace("redirect-gateway def1", "");

    // disable unsupported string on Unix systems
    m_ovpnFile.replace("block-outside-dns", "");

#ifdef Q_OS_MAC
#endif

    return m_ovpnFile;
}


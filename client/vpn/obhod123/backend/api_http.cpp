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
#include "pinger.h"

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

API_HTTP::AuthResult API_HTTP::requestAuth(const QString& user_id)
{
    QList<QPair<QString, double>> servers = Pinger::Instance().getServersSortedByPing();

    bool licenseNotFoundOnSomeServer = false;
    bool licenseExpiredOnSomeServer = false;
    for (auto p : servers) {
        QString ip = serversMap.value(p.first).value("ip").toString();
        if (ip.isEmpty()) continue;

        QString response = requestAuthWorker(user_id, ip);
        if (response == AUTH_SUCCESS) {
            qDebug() << "API_HTTP::requestAuth :: Successfully auth from" << ip;
            return AUTH_SUCCESS;
        }
        else if (response == AUTH_KEY_NOT_FOUND){
            qDebug() << "API_HTTP::requestAuth :: User id not found on ip:" << ip;
            // try to auth on another server
            licenseNotFoundOnSomeServer = true;
        }
        else if (response == AUTH_KEY_EXPIRED){
            qDebug() << "API_HTTP::requestAuth :: User id expired on ip:" << ip;
            // try to auth on another server
            licenseExpiredOnSomeServer = true;
        }
        else if (response == AUTH_TIMEOUT){
            qDebug() << "API_HTTP::requestAuth :: Request failed on" << ip;
        }
    }

    if (licenseNotFoundOnSomeServer) {
        // TODO
        // ask to init trial license
        return AUTH_KEY_NOT_FOUND;
    }
    if (licenseExpiredOnSomeServer) {
        // TODO
        // ask to init trial license
        return AUTH_KEY_EXPIRED;
    }

    return AUTH_TIMEOUT;
}

API_HTTP::AuthResult API_HTTP::requestAuthWorker(const QString& user_id, const QString& serverIp)
{
    // parse returned auth code as string
    // AUTH_SUCCESS, AUTH_TIMEOUT, AUTH_KEY_NOT_FOUND, AUTH_KEY_EXPIRED

    QUrlQuery q;
    q.addQueryItem("user_id", user_id);

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::requestAuthWorker:" << q.toString();
#endif

    QByteArray response_ba = decryptByteArray(GET_SYNC_ENCRYPTED(serverPrefix(serverIp), "c_auth", q, 3000));

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::requestAuthWorker :: Received users response: "<< response_ba;
#endif

    if (!QJsonDocument::fromJson(response_ba).object().contains("response")) return AUTH_TIMEOUT;

    QString response_text = QJsonDocument::fromJson(response_ba).object().value("response").toString();

    if (response_text == "AUTH_SUCCESS") {
        m_authData = QJsonDocument::fromJson(response_ba).object();
        m_user_id = user_id;
        return AUTH_SUCCESS;
    }
    if (response_text == "AUTH_KEY_NOT_FOUND") {
        return AUTH_KEY_NOT_FOUND;
    }
    if (response_text == "AUTH_KEY_EXPIRED") {
        return AUTH_KEY_EXPIRED;
    }

    return AUTH_TIMEOUT;
}

API_HTTP::ActivationResult API_HTTP::activateNewLicense(const QString &new_user_id, const QString &old_user_id)
{
    QList<QPair<QString, double>> servers = Pinger::Instance().getServersSortedByPing();

    for (auto p : servers) {
        QString ip = serversMap.value(p.first).value("ip").toString();
        if (ip.isEmpty()) continue;

        ActivationResult error = activateNewLicenseWorker(new_user_id, old_user_id, ip);
        if (error == ACTIVATION_SUCCESS) {
            qDebug() << "API_HTTP::activateNewLicense :: License activation successful on ip:" << ip;
            return ACTIVATION_SUCCESS;
        }
        else if (error == ACTIVATION_KEY_NOT_FOUND){
            qDebug() << "API_HTTP::activateNewLicense :: License activation key not found on ip:" << ip;
            return ACTIVATION_KEY_NOT_FOUND;
        }
        else if (error == ACTIVATION_KEY_EXPIRED){
            qDebug() << "API_HTTP::activateNewLicense :: License activation key expired on ip:" << ip;
            return ACTIVATION_KEY_EXPIRED;
        }
        else if (error == ACTIVATION_TIMEOUT){
            qDebug() << "API_HTTP::activateNewLicense :: License activation timeout on ip:" << ip;
        }
    }
    return ACTIVATION_TIMEOUT;
}

API_HTTP::ActivationResult API_HTTP::activateNewLicenseWorker(const QString &new_user_id, const QString &old_user_id, const QString &serverIp)
{
    // parse returned activation code as string
    // ACTIVATION_SUCCESS, ACTIVATION_TIMEOUT, ACTIVATION_KEY_NOT_FOUND, ACTIVATION_KEY_EXPIRED

    QUrlQuery q;
    q.addQueryItem("new_user_id", new_user_id);
    q.addQueryItem("old_user_id", old_user_id);

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::activateNewLicenseWorker:" << q.toString();
#endif

    QByteArray response_ba = decryptByteArray(GET_SYNC_ENCRYPTED(serverPrefix(serverIp), "c_activateKey", q, 3000));

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::activateNewLicenseWorker :: Received users response: "<< response_ba;
#endif

    if (!QJsonDocument::fromJson(response_ba).object().contains("response")) return ACTIVATION_TIMEOUT;

    QString response_text = QJsonDocument::fromJson(response_ba).object().value("response").toString();

    if (response_text == "ACTIVATION_SUCCESS") {
        m_authData = QJsonDocument::fromJson(response_ba).object();
        m_user_id = new_user_id;
        return ACTIVATION_SUCCESS;
    }
    if (response_text == "ACTIVATION_KEY_NOT_FOUND") {
        return ACTIVATION_KEY_NOT_FOUND;
    }
    if (response_text == "ACTIVATION_KEY_EXPIRED") {
        return ACTIVATION_KEY_EXPIRED;
    }

    return ACTIVATION_TIMEOUT;
}

bool API_HTTP::requestBlockedIps()
{
    QList<QPair<QString, double>> servers = Pinger::Instance().getServersSortedByPing();

    for (auto p : servers) {
        QString ip = serversMap.value(p.first).value("ip").toString();
        bool ok = requestBlockedIpsWorker(ip);
        if (ok) {
            qDebug() << "Successfully loaded blocked IP list from" << ip;

            // TODO now save list to disk
            //saveToDisk();
            return true;
        }
        else {
            qDebug() << "Load server list failed from" << ip;
        }
    }
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

bool API_HTTP::requestServers(const QList<QPair<QString, double>> &serverIds)
{
    // take as param list <id, ping> from pinger
    // ask closest servers for updated server list
    // save it to disk encrypted

    for (auto p : serverIds) {
        QString ip = serversMap.value(p.first).value("ip").toString();
        bool ok = requestServersWorker(ip);
        if (ok) {
            qDebug() << "Successfully loaded server list from" << ip;

            // now save downloaded servers
            saveServersToDisk();

            return true;
        }
        else {
            qDebug() << "Load server list failed from" << ip;
        }
    }
    return false;
}

bool API_HTTP::requestServersWorker(const QString& serverIp)
{
    QByteArray response = decryptByteArray(GET_SYNC_ENCRYPTED(serverPrefix(serverIp), "c_getServers", QUrlQuery(), 3000));

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::Received servers response: "<< response;
#endif

    QJsonArray sl = QJsonDocument::fromJson(response).object().value("servers").toArray();
    if (!sl.isEmpty()) {
        isServerListDownloaded = true;
        initServerListText(response);
        return true;
    }
    else return false;
}

bool API_HTTP::loadServersFromDisk()
{
    QFile file(serversListPath());
    file.open(QIODevice::ReadOnly);
    QByteArray ba_encrypted = file.readAll();
    file.close();

    QByteArray ba_decrypted = decryptByteArray(ba_encrypted);
    qDebug().noquote() << "Server list loaded from disk:" << ba_decrypted;

    return initServerListRaw(ba_decrypted);
}

bool API_HTTP::loadServersFromDisk_text()
{
    QFile file(serversListPath() + ".txt");
    file.open(QIODevice::ReadOnly);
    QByteArray ba = file.readAll();
    qDebug().noquote() << "Text servers file:" << serversListPath() + ".txt";
    qDebug().noquote() << "Server list loaded from disk (text):" << ba;

    //qDebug() << "Server list loaded from disk (text) converted to bson:" << QJsonDocument::fromJson(ba).toBinaryData();

    bool ok = initServerListText(ba);

    if (ok) {
        saveServersToDisk();
        return true;
    }
    else {
        return false;
    }
    //loadServersFromDisk();  // just for testing
}

void API_HTTP::saveServersToDisk()
{
    QByteArray ba_decrypted = QJsonDocument(serversObject).toBinaryData();

    QByteArray ba_encrypted = encryptByteArray(ba_decrypted);

    QFile file(serversListPath());
    file.open(QIODevice::WriteOnly);
    file.write(ba_encrypted);
    file.flush();
    file.close();
}

bool API_HTTP::initServerListRaw(const QByteArray &rawData)
{
    const QJsonObject &o = QJsonDocument::fromBinaryData(rawData).object();
    return initServerList(o);
}

bool API_HTTP::initServerListText(const QByteArray &textData)
{
    const QJsonObject &o = QJsonDocument::fromJson(textData).object();
    return initServerList(o);
}

bool API_HTTP::initServerList(const QJsonObject& o)
{
    QJsonArray a = o.value("servers").toArray();
    if (a.isEmpty()) {
        qDebug().noquote() << "API_HTTP::initServerList is empty, o:\n" << QJsonDocument(o).toJson();
        return false;
    }

    serversObject = o;

    const QJsonArray &serversList = serversObject.value("servers").toArray();

#ifdef API_HTTP_DEBUG
    qDebug() << "API_HTTP::initServerList, count" << serversList.count();
#endif

    serversMap.clear();
    regionsMap.clear();
    for (const QJsonValue &value : serversList) {
        const QJsonObject &server = value.toObject();
        regionsMap[server.value("region").toString()].append(server);
        serversMap[server.value("id").toString()] = server;
    }

    // Sort servers by alphabet
    for (const QString &region : regionsMap.keys()) {
        QVariantList serversArray = regionsMap.value(region).toVariantList();

        // Sort servers
        std::sort(serversArray.begin(), serversArray.end(), [](const QVariant &left, const QVariant &right){
            return left.toMap().value("name").toString() <  right.toMap().value("name").toString();
        });

        regionsMap[region] = QJsonArray::fromVariantList(serversArray);
    }

    qDebug() << "API_HTTP::initServerList :: servers map size" << serversMap.size();

    emit serverListChanged();

    return true;
}

QString API_HTTP::maintenancetool()
{
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath() + QDir::separator() + "obhod123_updater";
#endif

#ifdef Q_OS_MAC

#endif

    return QString();
}


void API_HTTP::requestDetectedLocation(std::function<void (QJsonObject)> func, int timeout)
{
    return;

    if (manager) manager->deleteLater();
    manager = new QNetworkAccessManager;
   // QUrl url("http://ipinfo.io/json");
   // QUrl url("http://ip-api.com/json");
    //QUrl url("http://34.212.229.4/geoip"); does't work

    QUrl url("http://52.26.142.80/geoip"); // taken from Windows version

#ifdef API_HTTP_DEBUG
    qDebug().noquote() << "API_HTTP::requestCurrentLocation";
#endif

    GET_ASYNC(url, [&, func](QByteArray response) {

#ifdef API_HTTP_DEBUG
     qDebug().noquote() << "API_HTTP::requestCurrentLocation response" << response;
#endif

        QJsonObject location = QJsonDocument::fromJson(response).object();
        func(location);
    }, timeout);
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



const QJsonArray API_HTTP::getServersInRegion(const QString& region) const
{
    return regionsMap.value(region);
}

const QJsonObject API_HTTP::getServerById(const QString& id) const
{  
    return serversMap.value(id);
}

bool API_HTTP::getServersListDownloaded() const
{
    return isServerListDownloaded;
}

QString API_HTTP::getUserId() const
{
    return m_user_id;
}

QJsonArray API_HTTP::getServersList() const
{
    return serversObject.value("servers").toArray();
}

const QStringList API_HTTP::getRegions() const
{
    QStringList regions = regionsMap.keys();
    std::sort(regions.begin(), regions.end());
    return regions;
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
    q.addQueryItem("user_id", m_user_id);

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


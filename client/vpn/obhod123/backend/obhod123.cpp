#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <QTimer>

#include "vpnutils.h"
#include "obhod123.h"
#include "vpndefine.h"

#ifdef Q_OS_WIN
#include "publib/winhelp.h"
#endif


QByteArray Obhod123::pageDataByUrl(const QString& url)
{
    m_network_error = false;
    QByteArray data;
    QNetworkAccessManager mgr;
    connect(&mgr, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)),this, SLOT(networkAccessibilityChanged(QNetworkAccessManager::NetworkAccessibility)));
    connect(&mgr, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),this, SLOT(sslErrorsCallback(QNetworkReply*,QList<QSslError>)));
    connect(&mgr, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),this, SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));

    QEventLoop loop;
    QNetworkReply* reply;

    reply = mgr.get(QNetworkRequest(QUrl(url)));
    connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(onNetworkError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    data = reply->readAll();
    reply->deleteLater();
    return data;
}

void Obhod123::networkAccessibilityChanged(QNetworkAccessManager::NetworkAccessibility accessible)
{
    qDebug().noquote() << "Network accessibility changed.";
    QNetworkReply* reply = (QNetworkReply*)this->sender();
    if (accessible == QNetworkAccessManager::NotAccessible)
    {
        if (reply && reply->isRunning())
        {
            qDebug().noquote() << "No network accessibility.";
            reply->close();
        }
    }
}

void Obhod123::onAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(authenticator);

    qDebug().noquote() << "Authentication required on request: " << reply->url().toString();
    if (reply) {
        reply->close();
    }
}

void Obhod123::sslErrorsCallback(QNetworkReply *reply, const QList<QSslError> &errors)
{
    qDebug().noquote() << "SSL errors occured.";
    if (reply)
    {
        reply->ignoreSslErrors();
        for (int i = 0; i < errors.count(); ++i)
            qDebug().noquote() << "SSL error: " << errors.at(i).errorString();
    }
}

void Obhod123::onNetworkError(QNetworkReply::NetworkError code)
{
    qDebug().noquote() << "Network error occured.";
    m_network_error = true;
    if (code != QNetworkReply::NoError)
    {
        QNetworkReply* reply = (QNetworkReply*)this->sender();
        int httpStatusCode = -1;
        if (reply && reply->isReadable())
            httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (code == QNetworkReply::TimeoutError ||
                code == QNetworkReply::ProxyTimeoutError ||
                code == QNetworkReply::ProtocolUnknownError ||
                httpStatusCode == 504) // timeout error
        {
            qDebug().noquote() << "Error operation time out";
        }
        else
        {
            qDebug().noquote() << "Unknown network error.Error code : " <<code ;
        }
        if (reply)
        {
            if (httpStatusCode == 511)
                qDebug().noquote() << "IP whitelisting error";
            reply->close();
        }
    }
}

Obhod123::Obhod123(QWidget *parent) :
    vpn(new VPNClient),
    autoConnectInProgress(false)
{
    Q_UNUSED(parent);

    m_vpnStatus = VPNStatusDisconnected;

    m_defgate = Obhod123Utils::getDefaultGateway();
    qDebug().noquote() << "GetDefaultGateway: " + m_defgate;

    messagesConnection = connect(vpn, SIGNAL(message(QString)), SLOT(onMessage(QString)));
    connect(vpn, SIGNAL(stateChanged(VPNClient::STATE)), SLOT(onStateChanged(VPNClient::STATE)));

    qsrand(QDateTime::currentDateTime().toMSecsSinceEpoch());

}

Obhod123::~Obhod123()
{
    if(m_vpnStatus == VPNStatusConnecting) {
        // cancel the connection.
        onCancel();
    }
    if(m_vpnStatus == VPNStatusConnected) {
        // disconnect the connection.
        onDisconnect();
    }

    delete vpn;
}

void Obhod123::onSetMTUSize()
{
    switch(vpn->type()) {
    case VPNClient::PPTP:
        Obhod123Utils::setMTUSize(VPNPPTPNAME, Settings::mtuSize()); break;
    case VPNClient::L2TP:
        Obhod123Utils::setMTUSize(VPNL2TPNAME, Settings::mtuSize()); break;
    case VPNClient::OPENVPN:
        Obhod123Utils::setMTUSize(VPNNAME, Settings::mtuSize()); break;
    case VPNClient::CAMOUFLAGE:
        Obhod123Utils::setMTUSize(VPNNAME, Settings::mtuSize()); break;
    default:
        break;
    }
}


int Obhod123::getPingByHostName(const QString& host, int count, qreal &loss)
{
    QProcess ping;
#ifdef Q_OS_MACX
    ping.start("ping", QStringList() << QString("-c%1").arg(count) << host);
#endif
#ifdef Q_OS_WIN
    ping.start( "ping", QStringList() << QString("/n") << QString::number(count) << host);
#endif
    ping.waitForStarted();

    QEventLoop loop;
    loop.connect(&ping, SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.connect(this, SIGNAL(testCancel()), &loop, SLOT(quit()));
    loop.exec();

    QByteArray d = ping.readAll();
    if(d.size() == 0)
        return -1;
#ifdef Q_OS_MACX
    loss = Obhod123Utils::getStringBetween(d, "received, ", "% packet").toDouble();
    QStringList s = Obhod123Utils::getStringBetween(d, "stddev = ", " ms").split("/");
    return (s.count() != 4 ? 0 : (s.at(1).toDouble()));
#endif
#ifdef Q_OS_WIN   
    loss = Obhod123Utils::getStringBetween(d, " (", "% loss)").toDouble();
    return Obhod123Utils::getStringBetween(d, "time=", "ms").toDouble();
#endif
}

Obhod123::VPNStatus Obhod123::vpnStatus() const
{
    return m_vpnStatus;
}

QString Obhod123::vpnGate() const
{
    return m_vpngate;
}

void Obhod123::setVpnGate(const QString &vpnGate)
{
    m_vpngate = vpnGate;
}


VPNClient::TYPE Obhod123::getTypeByIndex(int index)
{
#ifdef Q_OS_WIN
    switch(index) {
    case 0: return VPNClient::L2TP;
    case 1: return VPNClient::PPTP;
    case 2: return VPNClient::SSTP;
    case 3: return VPNClient::OPENVPN;
    case 4: return VPNClient::CAMOUFLAGE;
    case 5: return VPNClient::OPENVPN;
    case 6: return VPNClient::OPENVPN;
    }
#endif

#ifdef Q_OS_MAC
    switch(index) {
    case 0: return VPNClient::L2TP;
    case 1: return VPNClient::PPTP;
    case 2: return VPNClient::SSTP;
    case 3: return VPNClient::OPENVPN;
    }
#endif
    return VPNClient::INVALID;
}

VPNClient::TYPE Obhod123::getTypeByName(const QString& protocolName)
{
    if (protocolName == "l2tp") return VPNClient::L2TP;
    else if (protocolName == "pptp") return VPNClient::PPTP;
    else if (protocolName == "sstp") return VPNClient::SSTP;
    else if (protocolName == "ovpn") return VPNClient::OPENVPN;
    else if (protocolName == "tcp") return VPNClient::OPENVPN;
    else if (protocolName == "udp") return VPNClient::OPENVPN;
    else if (protocolName == "camo") return VPNClient::CAMOUFLAGE;

    else return VPNClient::INVALID;
}

int Obhod123::getIndexByType(VPNClient::TYPE type) const
{
    if(type == VPNClient::L2TP)
        return 0;
    if(type == VPNClient::PPTP)
        return 1;
    if(type == VPNClient::SSTP)
        return 2;
    if(type == VPNClient::OPENVPN)
        return 3;

    return -1;
}

bool Obhod123::onConnectAuto(const QJsonObject& connectionParams)
{
    if (m_vpnStatus == VPNStatusConnected) {
        qDebug().noquote() << "Obhod123::onConnectAuto :::: Step 1: already connected, exiting";
        return true;
    }

    cancelAutoConnect = false;
    autoConnectInProgress = false;   //set to true in the loop

    qDebug().noquote() << "Obhod123::onConnectAuto ___START___";

    const QJsonObject &server = API_HTTP::Instance().getCurrentServer();


    disconnect(messagesConnection); // don't diplay messages when auto connect

    int count = 0;
    forever {
        autoConnectInProgress = true;
        qDebug().noquote() << "Obhod123::onConnectAuto :::: loop start";
        if (cancelAutoConnect) break;

        if (m_vpnStatus == VPNStatusDisconnecting || m_vpnStatus == VPNStatusCanceling) {
            qDebug().noquote() << "Obhod123::onConnectAuto :::: break on  VPNStatusDisconnecting | VPNStatusCanceling";
            break;
        }
        if (m_vpnStatus == VPNStatusConnected) {
            qDebug().noquote() << "Obhod123::onConnectAuto :::: break on VPNStatusConnected";
            break;
        }

        count++;

        QJsonObject connParams = connectionParams;
        connParams.insert("server", server);

        emit autoTryingServer(server.value("id").toString(), count);

        // test
        //        if (qrand() % 5 > 10) {
        //            QJsonObject server = servers.first();
        //            server.insert("ip", "google.com");
        //            connParams.insert("server", server);
        //        }

        qDebug().noquote() << "Trying to connect with params:" << connParams;
        bool ok = onConnect(connParams);

        QEventLoop loop;
        QTimer::singleShot(1000, &loop, SLOT(quit()));
        loop.exec();
        if (ok) {
            autoConnectInProgress = false;
            return true;
        }
    }
    qDebug().noquote() << "Obhod123::onConnectAuto :::: exited";
    autoConnectInProgress = false;

    disconnect(messagesConnection); // disconnect again: don't diplay messages when auto connect
    messagesConnection = connect(vpn, SIGNAL(message(QString)), SLOT(onMessage(QString))); // enable messages

    if (m_vpnStatus == VPNStatusConnected) {
        qDebug().noquote() << "Obhod123::onConnectAuto :::: Step 2: already connected, exiting";
        return true;
    }

    if (cancelAutoConnect) return true;
    return false;
}

void Obhod123::onClickDisconnect()
{
    onDisconnect();
}

bool Obhod123::onConnect(const QJsonObject& connectionParams)
{
    if (m_vpnStatus == VPNStatusConnected) {
        qDebug().noquote() << "Obhod123::onConnect :::: exit on VPNStatusConnected";
        return true;
    }

    vpn = new VPNClient;
    messagesConnection = connect(vpn, SIGNAL(message(QString)), SLOT(onMessage(QString)));
    connect(vpn, SIGNAL(stateChanged(VPNClient::STATE)), SLOT(onStateChanged(VPNClient::STATE)));


    m_connectionParams = connectionParams;
    const QJsonObject &server = m_connectionParams.value("server").toObject();
    QString serverIp = server.value("ip").toString();

    // -----------------------

    if(vpn->isRunning()) {
        qDebug() << "Obhod123::onConnect :::: vpn->isRunning() == true -> return false";
        return false;
    }

    QString protocol = m_connectionParams.value("protocol").toString();

    VPNClient::OPTION o;
    o.type = getTypeByName(protocol);
    //o.name = m_connectionParams.value("username").toString();
    //o.pass = "";
    o.setup.type = getTypeByName(protocol);
    switch(o.type) {
    case VPNClient::OPENVPN:
    case VPNClient::CAMOUFLAGE:	{
        o.conn = API_HTTP::Instance().downloadOVPNConfiguration(server, protocol);
        if (o.conn.isEmpty()) {
            qDebug().noquote() << "Error occurred downloading OVPN file. Reseting NAM";
            API_HTTP::Instance().resetNAM();

            return false;
        }

        o.setup.name = "update";

        o.setup.ip = Obhod123Utils::getIPFromOvpn(o.conn);
        if (o.setup.ip.isEmpty()) {
            qDebug().noquote() << "Error occurred parsing OVPN file.";
            return false;
        }
        qDebug().noquote() << "setup.ip: " + o.setup.ip;

        // proxy setting about openvpn.
        if(Settings::proxyType() == "http")
            o.ext = QString("--http-proxy|%1|%2")
                    .arg(Settings::proxyAddress())
                    .arg(Settings::proxyPort());

        if(Settings::proxyType() == "socks5")
            o.ext = QString("--socks-proxy|%1|%2")
                    .arg(Settings::proxyAddress())
                    .arg(Settings::proxyPort());
        break;
    }
    case VPNClient::PPTP:
    case VPNClient::L2TP:
        if (serverIp.isEmpty()) return false;
        o.setup.ip = serverIp;
        break;

    case VPNClient::SSTP:
        o.setup.ip = serverIp;
        break;
    default:
        return false;
    }

    // reset current ip, and we will get the ip later.
    curip.clear();

    // set current connect button to disable.
    setVpnStatus(VPNStatusConnecting);
    if(!vpn->setup(o.setup)) {
        setVpnStatus(VPNStatusDisconnected);
        qDebug() << "Obhod123::onConnect, cannot set vpn->setup()";
        return false;
    }

    // HERE POINT WHERE VPN CONNECTED
    m_srvip = o.setup.ip;

    if(Settings::networkLock()) {
        // implement me
    }

    if(!vpn->open(o)) {
        setVpnStatus(VPNStatusDisconnected);
        qDebug() << "Obhod123::onConnect, cannot set vpn->open()";
        return false;
    }


    if(m_vpnStatus == VPNStatusCanceling || m_vpnStatus == VPNStatusDisconnected) {
        vpn->close();
        setVpnStatus(VPNStatusDisconnected);

        return false;
    }

    setVpnStatus(VPNStatusConnected);

    if (o.type == VPNClient::OPENVPN ||o.type == VPNClient::CAMOUFLAGE ) {
        m_vpngate = Obhod123Utils::getOvpnGateway();
    }
    else {
        m_vpngate = Obhod123Utils::getCurrentVPNConfigIP();
    }

    QTimer::singleShot(500, [&, o](){
        if(Settings::appMode() == "obhod") {
            // Setup routes
            Router::Instance().clearSavedRoutes();

            // Implement me - manage DNS
            // install routes to dns
            Router::Instance().routeDelete("8.8.8.8");
            Router::Instance().routeDelete("8.8.8.4");

            Router::Instance().flushDns();

            const QStringList &black = API_HTTP::Instance().getBlackIPs();
            qDebug() << "Obhod123::onConnect :: adding server black routes, count:" << black.size();
            Router::Instance().routeAddList(m_vpngate, black);

            const QStringList &black_custom = Settings::customIps();
            qDebug() << "Obhod123::onConnect :: adding custom black routes, count:" << black_custom.size();
            Router::Instance().routeAddList(m_vpngate, black_custom);

            // Setup DNS
            NetworkController::setDnsServersOnHardwareAdapters("8.8.8.8", "8.8.8.4");
            Router::Instance().flushDns();
        }
    });


    QTimer::singleShot(0, this, SLOT(onUpdateTray()));

    return true;
}

void Obhod123::onDisconnect()
{
    setVpnStatus(VPNStatusDisconnecting);
    vpn->close();
#ifdef Q_OS_MAC
    VPNClient::OPTION o;
    //o.type = getTypeByIndex(ui->cbProtocol->currentIndex());
    vpn->deleteNetworkPreference(o.type);
#endif
    setVpnStatus(VPNStatusDisconnected);
    Router::Instance().clearSavedRoutes();
}

void Obhod123::onCancel()
{
    cancelAutoConnect = true;  // dont try connect to next servers

    setVpnStatus(VPNStatusCanceling);
    vpn->close();
    setVpnStatus(VPNStatusDisconnected);
}

void Obhod123::onMessage(const QString& message)
{
    emit vpnClientMessage(message);

    QString msg = message;
    qDebug().noquote() << msg << "\r\n";

    if(msg.contains("route-gateway ")) {
        int pos = msg.indexOf("route-gateway") + sizeof("route-gateway");
        m_vpngate = msg.mid(pos).split(",").at(0);
    }

    if(msg.contains(QString("[ERROR]") + tr("VPN user name or password incorrect."))
            ||(msg.contains("691") && msg.contains("[ERROR]")))
        msg = QString("[ERROR]") + tr("VPN User name or Password incorrect.");

#ifdef Q_OS_MAC
    if(vpn->type() != VPNClient::OPENVPN && vpn->type() != VPNClient::SSTP)
        return;
#endif

    if(msg.contains("[ERROR]")) {
        if(!Settings::reconnectOnDrop()) {
            // TODO: add messages on error
            // QMessageBox::critical(0, "Error", msg.mid(7));
            qDebug() << "Obhod123::onMessage :: got error : " << msg;
        }
        else {
            setVpnStatus(VPNStatusDisconnecting);
            vpn->close();
            setVpnStatus(VPNStatusDisconnected);
            //QTimer::singleShot(0, this, SLOT(onConnect()));
        }
        return;
    }
}

void Obhod123::onStateChanged(VPNClient::STATE s)
{
    qDebug() << "Obhod123::onStateChanged, new state:" << s;
    emit vpnClientStateChanged(s);
    if(s == VPNClient::VPNERROR) {
        vpn->close();
        setVpnStatus(VPNStatusDisconnected);

        Router::Instance().clearSavedRoutes();
    }
}

//void Obhod123::onTimer()
//{
//    if(curip.isNull()) {
//        curip = Obhod123Utils::getIPFromRemote();
//    }
//}

void Obhod123::reset()
{
    if(m_vpnStatus == VPNStatusConnecting) {
        // cancel the connection...  I do not care...
    }
    if(m_vpnStatus == VPNStatusConnected) {
        // disconnect the connection.
        onDisconnect();
    }
    qApp->exit();
}


void Obhod123::onUpdateTray()
{
    if(vpn->state() != VPNClient::CONNECTED)
        m_tray.setToolTip(tr("Obhod123 (Not Connected)"));
}

QString Obhod123::VPNStatusString(enum VPNStatus status)
{
    switch (status) {
    case VPNStatusInit: return "Init";
    case VPNStatusConnecting: return "Connecting";
    case VPNStatusConnected: return "Connected";
    case VPNStatusDisconnecting: return "Disconnecting";
    case VPNStatusDisconnected: return "Disconnected";
    case VPNStatusCanceling: return "Cancelling";
    case VPNStatusError: return "Error";
    case VPNStatusCount: return "Count";
    default:
        break;
    }
    return QString();
}

void Obhod123::setVpnStatus(Obhod123::VPNStatus s)
{
    qDebug().noquote() << "Obhod123::setStatus: " << VPNStatusString(s);

    m_vpnStatus = s;
}



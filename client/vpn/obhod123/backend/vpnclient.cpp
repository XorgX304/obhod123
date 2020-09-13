#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>

#include "vpnclient.h"
#include "vpndefine.h"

#ifdef Q_OS_MAC
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include "publib/winhelp.h"
#endif

static QString VPNMGRPORT = "44358";
static const QString VPNMGRPORTMAX = "45058";
static const quint32 TIMEOUTFIX = 2000;


// implement helper tools for other protocols
QString VPNCOMPOSE(VPNClient::TYPE protocol)
{
    if (protocol == VPNClient::OPENVPN) {
        // Open VPN
        return "/openvpn.exe";
    }
    else if (protocol == VPNClient::PPTP || protocol == VPNClient::L2TP) {
        // PPTP L2TP
        return "/l2tp_manager.exe";
    }
    else if (protocol == VPNClient::SSTP) {
        // SSTP
        return "/sstp_manager.exe";
    }
    else if (protocol == VPNClient::IKEV2) {
        // IKEv2
        return "/ikev2_manager.exe";
    }
    else if (protocol == VPNClient::CAMOUFLAGE) {
        // CAMOUFLAGE
        return "/custom_protocol_manager";
    }
    else {
        qDebug() << "Requested wrong protocol:" << protocol;
        return "";
    }
}

const QString SETUPCOMPOSE[] = {
    "/drv/x${SYSTEM_TYPE}/tapinstall.exe",
    "/compose4",         // PPTP
    "/compose4",         // L2TP
    "/compose4",         // SSTP
    "/compose4",         // IKEv2
    "/composecn",        // CAMOUFLAGE
};

const QString TYPESTRING[] = {
    "OPENVPN",
    "PPTP",
    "L2TP",
    "SSTP",
    "IKEV2",
    "CAMOUFLAGE",
};

const QString DISPLAYSTRING[] = {
    VPNNAME,
    VPNPPTPNAME,
    VPNL2TPNAME,
    VPNSSTPNAME,
    VPNIKEV2NAME,
};

const quint32 CHECKLOOP = 50;

static QString FormatCompose(const QString& path)
{
#ifdef Q_OS_WIN
    // 64 and 32 bits sometimes use different path.
    QString p = path;
    p.replace("${SYSTEM_TYPE}", QString::number(winhelpSystemBits()));
#endif
    return path;
}

static bool UpdateValidManagementPort()
{
    QTcpServer sock;
    quint16 port = (quint16)VPNMGRPORT.toUShort();
    quint16 max  = (quint16)VPNMGRPORTMAX.toUShort();
    for(; port < max; port++) {
        if(!sock.listen(QHostAddress::LocalHost, port))
            continue;
        VPNMGRPORT = QString::number(port);
        return true;
    }
    // no valid port.
    return false;
}

VPNClient::VPNClient()
{
    m_timeout = 60000;
    m_state = DISCONNECTED;
    m_compose = nullptr;
    m_rcv = m_snd = 0;
    m_agent = nullptr;

    connect(this, &VPNClient::managementMessage, [&](QString message){
         qDebug() << "VPNClient MESSAGE:" << message;
         if (message.contains("UPDOWN:DOWN")) {
             qDebug() << "VPNClient MESSAGE: UPDOWN:DOWN - disable VPN by setting VPN ERROR";
             setState(VPNERROR);
         }
    });
}

VPNClient::~VPNClient()
{
    close();
}

void VPNClient::run()
{
    //qDebug() << "VPNClient::run :::: begin";
    m_compose = new QProcess;
    m_compose->setProcessChannelMode(QProcess::MergedChannels);

    switch(m_option.type) {
        case OPENVPN:
        case CAMOUFLAGE:	{
            // start the connection.
            QStringList parameter;
           // parameter << "--cipher" << "AES-256-CBC";

            parameter << "--redirect-private"; // << "bypass-dns";
            parameter << "--verb" << "4";

            // pushed from server
            //parameter << "--ping" << "10";
            //parameter << "--ping-restart" << "60";

            parameter << "--ping-exit" << "30";

            parameter << "--management-up-down";
            parameter << "--management-signal";





            if(!m_option.conn.isEmpty()) {
                parameter << "--config";
                QString tempOvpnFile = Obhod123Utils::saveConfigToTempFile(m_option.conn);
                parameter << tempOvpnFile;
            }
            if(!m_option.name.isEmpty() || !m_option.pass.isEmpty()) {
                parameter << "--auth-user-pass";
    #ifdef Q_OS_MAC
                // under linux, we must enable read from file.
                parameter << qApp->applicationDirPath() + "/login";
                QFile login(qApp->applicationDirPath() + "/login");
                login.open(QIODevice::Truncate | QIODevice::WriteOnly);
                login.write((m_option.name + "\r\n").toLocal8Bit());
                login.write((m_option.pass + "\r\n").toLocal8Bit());
                login.close();
    #endif
                parameter << "--auth-nocache";
            }
            if(!m_option.ext.isEmpty()) {
                parameter << m_option.ext.split("|", QString::SkipEmptyParts);
            }
            // management option, we need to get information by this set.
            if(UpdateValidManagementPort()) {
                parameter << "--management" << "127.0.0.1" << VPNMGRPORT;
                emit message(QString("Use port ") + VPNMGRPORT + " as management port");
            }
    #ifdef Q_OS_WIN
            HANDLE winevent = NULL;
            QString wineventname;
            qsrand(GetTickCount());
            wineventname= QString("VPNCLIENTEVENT") + QString::number(qrand());
            winevent = CreateEventA(NULL, TRUE, FALSE, wineventname.toLocal8Bit());
            parameter << "--service";
            parameter << wineventname << "0";
    #endif

            if(m_option.type == OPENVPN)
                m_compose->start(composeDir() + FormatCompose(VPNCOMPOSE(OPENVPN)), parameter);
            else
                m_compose->start(composeDir() + FormatCompose(VPNCOMPOSE(CAMOUFLAGE)), parameter);

            if(!m_compose->waitForStarted()) {
                emit message(QString("[ERROR]") + tr("start openvpn process failed."));
                setState(VPNERROR);
                return;
            }

            while(1) {
                if (!m_compose) return;
                if(m_compose->state() == QProcess::NotRunning) {
                    emit message(m_compose->readAll());
                    break;      // wait until the process is end.
                }
                if(m_compose->waitForReadyRead(CHECKLOOP)) {
                    QString d = m_compose->readAll();
                    emit message(d);
                    if(d.contains("Auth Username:")) {
                        m_compose->write(m_option.name.toLocal8Bit());
                        m_compose->write("\r\n");
                        continue;
                    }
                    if(d.contains("Auth Password:")) {
                        m_compose->write(m_option.pass.toLocal8Bit());
                        m_compose->write("\r\n");
                        continue;
                    }
                    if(d.contains("AUTH_FAILED")) {
                        emit message(QString("[ERROR]") + tr("username or password is not matched."));
                        setState(VPNERROR);
                        continue;
                    }
                    if(d.contains("Initialization Sequence Completed")) {
                        setState(CONNECTED);
                        continue;
                    }
    #ifdef Q_OS_MAC
                    if(d.contains("Peer Connection Initiated")) {
                        // login file is useless now, delete for more safity.
                        QFile(qApp->applicationDirPath() + "/login").remove();
                    }
    #endif
                }
                if(state() == DISCONNECTING) {   // connection is canceled or disconnected.
    #ifdef Q_OS_WIN
                    // call stop event to stop openvpn.
                    SetEvent(winevent);
    #endif

    #ifdef Q_OS_MAC
                    // we just kill it, when it receive SIGTERM, it will be end.
                    m_compose->kill();
    #endif

    #ifdef Q_OS_LINUX
                    // we just kill it, when it receive SIGTERM, it will be end.
                    m_compose->kill();
    #endif
                    // if the system is unknown, we will terminate it in close after time out.
                    continue;                    // loop will end when the process is end.
                }
            }

            // vpn process is end, we need to check the result.
            if(state() == CONNECTED) {
                emit message(QString("[ERROR]") + tr("Obhod123 disconnected"));
                setState(VPNERROR);
            }
            if(state() == CONNECTING) {
                emit message(QString("[ERROR]") + tr("Obhod123 disconnected"));
                setState(VPNERROR);
                // we do not change the state, keep in old way until call close.
            }

    #ifdef Q_OS_WIN
            CloseHandle(winevent);
    #endif
            break; }

        case SSTP:
        case PPTP:
        case L2TP:
        case IKEV2: {
            // Implement me for other protocols
        }
    }
}

bool VPNClient::open(VPNClient::OPTION o)
{
    //forceDisconnectWanMiniPort(o.type);

    //qDebug().noquote() << "VPNClient::open";
    if(o.type <= INVALID || o.type >= TYPEEND)
        return false;       // vpn is invalid.
    if(state() != DISCONNECTED && state() != VPNERROR)
        return false;       // vpn is already opened.
    m_option = o;

    // check compose for the protocol.
    if(!QFile::exists(composeDir() + FormatCompose(VPNCOMPOSE(o.type)))) {
        emit message(QString("[ERROR]") + tr("openvpn compose is missed. File is ") + (composeDir() + FormatCompose(VPNCOMPOSE(o.type))) );
        setState(VPNERROR);
        return false;
    }

    // init agent thread, it is used to monitor connection state.
    if(m_agent != nullptr)
        m_agent->deleteLater();
    m_agent = new VPNAgent;
    m_agent->setClient(this);

    // everything is all right, now start connection.
    setState(CONNECTING);
    if(m_compose != nullptr)
    {
        m_compose->deleteLater();
        m_compose = nullptr;
    }

    m_try_count = 0;
    //qDebug().noquote() << "VPNClient::open :::: Entering loop";

loop:
    m_unknown_received = false;

    //qDebug().noquote() << "VPNClient::open :::: Loop Start, m_try_count =" << m_try_count;

    exit(0);
    wait(m_timeout);

    terminate();
    wait(m_timeout);

    //qDebug().noquote() << "VPNClient::open :::: Trying to start, isRunning:" << isRunning();
    start();

    //qDebug().noquote() << "VPNClient::open :::: Started:" << isRunning();
    m_try_count += 1;

    //qDebug().noquote() << "VPNClient::open :::: QEventLoop loop.exec before, m_timeout =" << m_timeout;

    // wait for connected.
    QPointer<QEventLoop> loop = new QEventLoop;
    QPointer<QTimer>     checkTimer = new QTimer;

    QDateTime  cur = QDateTime::currentDateTime();
    //QTimer     timer;
    //timer.singleShot(m_timeout, &loop, SLOT(quit()));
    QTimer::singleShot(m_timeout, loop.data(), SLOT(quit()));
    connect(this, &VPNClient::finished, loop.data(), &QEventLoop::quit);
    connect(this, &VPNClient::connected, loop.data(), &QEventLoop::quit);
    connect(this, &VPNClient::vpnunknown, this, &VPNClient::vpnunknownreceivedcallback);
    connect(this, &VPNClient::vpnunknown, loop.data(), &QEventLoop::quit);

    // also check every ~1 sec if thread is running
    // may be it was terminated
    auto c = connect(checkTimer, &QTimer::timeout, [&, loop, checkTimer](){
        if (!isRunning()) {
            if (checkTimer) checkTimer->stop();
            if (loop) loop->quit();
            qDebug() << "Detected VPN thread is not running. break";
        }
    });
    checkTimer->start(500);
    loop->exec();

    disconnect(c);
    if (checkTimer) checkTimer->deleteLater();
    if (loop) loop->deleteLater();

    //qDebug().noquote() << "VPNClient::open :::: QEventLoop loop.exec after";

    if(m_unknown_received)
    {
        if(m_try_count < 40)
        {
            goto loop;
        }
    }
    // return vpn thread state.
    if(cur.msecsTo(QDateTime::currentDateTime()) < m_timeout - TIMEOUTFIX)
        return isRunning();

    // timeout is triggered.
    emit message(QString("[ERROR]") + tr("Connect to server timeout"));
    setState(VPNERROR);
    return false;
}

void VPNClient::vpnunknownreceivedcallback()
{
    m_unknown_received = true;
}

bool VPNClient::close()
{
    if(state() == DISCONNECTED || state() == DISCONNECTING || !isRunning())
        return false;       // not connected or disconnect is in process.
    setState(DISCONNECTING);
    emit message("disconnecting from the network...\n");

    // clear up last connection, wait for it is done.
    QEventLoop loop;
    QTimer     timer;
    timer.singleShot(m_timeout, &loop, SLOT(quit()));

    connect(this, &VPNClient::finished, &loop, &QEventLoop::quit);

    if(isRunning())
        loop.exec();


    // if it is still running not end, kill it.
    if(m_compose != nullptr) {
        if(m_compose->state() == QProcess::Running)
            m_compose->kill();
        m_compose->deleteLater();
        m_compose = nullptr;
    }

    if(m_agent != nullptr) {
        m_agent->deleteLater();
        m_agent = nullptr;
    }

    m_option.type = INVALID;
    setState(DISCONNECTED);

    disconnect(this, 0, 0, 0);

    return true;
}

#ifdef Q_OS_WIN
bool VPNClient::setup(VPNClient::SETUP s)
{
    if (s.type == OPENVPN || s.type == CAMOUFLAGE) {
        if (! TapController::Instance().checkInstaller()) {
            emit message("Setup tap driver file missing.");
            return false;
        }
        if (! TapController::Instance().checkAndSetup()) {
            emit message("Setup tap driver file failed.");
            return false;
        }
    }

    else if (s.type == PPTP || s.type == SSTP || s.type == L2TP || s.type == IKEV2) {
        QStringList p;

        p.append(TYPESTRING[s.type]);
        p.append(DISPLAYSTRING[s.type]);
        p.append(s.ip);
        if(s.type == L2TP) {
            s.key = VPNKEY4L2TP;
            p.append(s.key);
        }
        if(s.type == SSTP && !s.key.isEmpty())
            p.append(s.key);

        bool success = true;
        QByteArray log;
        emit message("Checking local settings...");

        int tryCount = -1;
        bool continueTrying = true;

        while(continueTrying)
        {
            tryCount +=1;
            // Implement me
            // Setup adapter for other protocols
            continueTrying = false;
        }

        emit message("Setup local driver done.");
    }

    else return false;

    return true;
}
#endif

QList<QByteArray> VPNClient::getAllNetworkServices() const
{
    QProcess p;
    QString program = QString("networksetup -listallnetworkservices") ;
    p.start(program);
    p.waitForFinished();
    QByteArray networkListBytes = p.readAll();
    QList<QByteArray> networkList = networkListBytes.split('\n');
    return networkList;
}

void VPNClient::deleteNetworkPreference(TYPE type)
{
    QString protocol = DISPLAYSTRING[type];
    QList<QByteArray> networkPreferenceList = getAllNetworkServices();
    for(int index=0; index<networkPreferenceList.count();index++)
    {
        QString networkPreferenceProtocol = networkPreferenceList.at(index).trimmed();
        if(networkPreferenceProtocol.contains(protocol,Qt::CaseInsensitive))
        {
            // delete from network preference
            //networksetup -deletepppoeservice "ibVPN-PPTP"
            QString program = QString("%1 -%2 \"%3\"").arg("networksetup").arg("deletepppoeservice").arg(networkPreferenceProtocol.trimmed());
            executeProgram(program);
        }
    }
}

void VPNClient::executeProgram(const QString& program)
{
    QProcess p;
    p.start(program);
    p.waitForFinished();
}

bool VPNClient::unsetup(VPNClient::SETUP s)
{
    switch(s.type) {
    case OPENVPN:
    case CAMOUFLAGE:{
#ifdef Q_OS_WIN
        QString exe = qApp->applicationDirPath() + FormatCompose(SETUPCOMPOSE[OPENVPN]);
        if(s.type == CAMOUFLAGE)
            exe = qApp->applicationDirPath() + FormatCompose(SETUPCOMPOSE[OPENVPN]);
        QProcess setup;
        setup.start(exe, QStringList() << "remove" << "obhod123tap");
        if(!setup.waitForStarted())
            return false;

        while(1) {
            if(setup.state() == QProcess::NotRunning)
                break;      // wait until the process is end.
            if(setup.waitForReadyRead(CHECKLOOP))
                emit message(setup.readAll());
        }
#endif
        break; }

    case PPTP: case L2TP: case SSTP: case IKEV2: {
        QProcess setup;
        setup.start(composeDir() + SETUPCOMPOSE[s.type],
                QStringList() << TYPESTRING[s.type] << DISPLAYSTRING[s.type]);
        if(!setup.waitForStarted())
            return false;
        emit message("Remove local driver...");

        QEventLoop loop;
        QTimer     timer;
        timer.singleShot(m_timeout, &loop, SLOT(quit()));
        loop.connect(&setup, SIGNAL(finished(int)), SLOT(quit()));
        loop.exec();

        QByteArray log = setup.readAll();
        emit message(log);

        if(!log.contains("Success")) {
            emit message(QString("[ERROR]") + tr("Remove local driver failed."));
            setState(VPNERROR);
            return false;
        }
        emit message("Remove local driver done.");
        break; }

    default: {
        return false; }
    }
    return true;
}

VPNClient::STATE VPNClient::state()
{
    return m_state;
}

void VPNClient::setState(STATE state)
{
    if(m_state == state)
    {
        if(state == VPNUNKNOWN)
            emit vpnunknown();
        return;
    }
    m_state = state;
    emit stateChanged(m_state);
    switch(m_state) {
    case CONNECTING: emit connecting(); break;
    case DISCONNECTED: emit disconnected(); break;
    case DISCONNECTING: emit disconnecting(); break;
    case VPNERROR: emit vpnerror(); break;
    case VPNUNKNOWN: emit vpnunknown();break;
    case CONNECTED: {
        emit connected();
        if(m_agent != nullptr) {
            m_agent->start();
        }
        break;
    }
    }
}

VPNClient::TYPE VPNClient::type()
{
    return m_option.type;
}

bool VPNClient::reset()
{
    bool r = true;
    r = unsetup(SETUP(OPENVPN)) && r;
    return r;
}

void VPNClient::getByteCount(quint64 &receive, quint64 &send)
{
    receive = m_rcv;
    send = m_snd;
}
void VPNClient::setByteCount(quint64 receive, quint64 send)
{
    m_rcv = receive;
    m_snd = send;
}

QString VPNClient::composeDir()
{
#ifdef Q_OS_WIN
    return qApp->applicationDirPath() + "/openvpn/bin/";
#endif

#ifdef Q_OS_MAC
    //return qApp->applicationDirPath() + "/resources_mac/";
    return qApp->applicationDirPath() + "/";
#endif
}

VPNAgent::VPNAgent() :
    m_vpnClient(NULL)
{
}

void VPNAgent::setClient(VPNClient *p)
{
    m_vpnClient = p;
}

void VPNAgent::run()
{
    if(m_vpnClient == NULL || m_vpnClient->state() != VPNClient::CONNECTED) {
        // pointer must be valid.
        return;
    }

    switch(m_vpnClient->m_option.type) {
    case VPNClient::OPENVPN:
    case VPNClient::CAMOUFLAGE:{
        QTcpSocket sock;
        sock.connectToHost(QHostAddress("127.0.0.1"), VPNMGRPORT.toUShort());
        sock.waitForConnected();
        sock.waitForReadyRead();

        QByteArray data = sock.readAll();
        sock.write("bytecount 1\r\n");
        sock.waitForReadyRead();
        data = sock.readAll();

        while(m_vpnClient->state() == VPNClient::CONNECTED) {
            sock.waitForReadyRead();
            data = sock.readAll();




            int beg = data.lastIndexOf(">BYTECOUNT:");
            int end = data.indexOf("\n", beg);
            if(beg < 0) {
                emit m_vpnClient->managementMessage(QString(data));
                //qDebug() << "VPNAgent message:" << data;
                continue;
            }

            beg += sizeof(">BYTECOUNT:") - 1;
            QList<QByteArray> count = data.mid(beg, end - beg + 1).split(',');

            quint64 r = (quint64)count.at(0).trimmed().toULongLong();
            quint64 s = (quint64)count.at(1).trimmed().toULongLong();
            m_vpnClient->setByteCount(r, s);
        }
        break;
    }

    default:
        return;
    }
}


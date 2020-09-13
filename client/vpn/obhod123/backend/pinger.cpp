#include "pinger.h"
#include "api_http.h"

Pinger::Pinger()
{
    connect(&checkTimer, &QTimer::timeout, this, &Pinger::checkServersPing);

    startServersPing();

#ifdef Q_OS_MAC
   connect(&pingerObject, SIGNAL(pingFinished(const QString&, const QString&, int)),  this, SLOT(pingFinished(const QString&, const QString&, int)));
#endif
}

#ifdef Q_OS_MAC
 void Pinger::pingFinished(const QString& ip, const QString& data, int delay)
 {
     Q_UNUSED(ip);
     registerPingForServer(data, delay);
 }
#endif

Pinger &Pinger::Instance()
{
    static Pinger s;
    return s;
}

void Pinger::startServersPing()
{
    QTimer::singleShot(0, [&] (){
        setPingCheckingEnabled(true);
    });
    checkTimer.start(1000);
}

void Pinger::stopServersPing()
{
    setPingCheckingEnabled(false);
    checkTimer.stop();
}

bool Pinger::isPingCheckingEnabled() const
{
    return m_isPingCheckingEnabled;
}

void Pinger::setPingCheckingEnabled(bool value)
{
    qDebug() << "Pinger::setPingCheckingEnabled set to:" << value;
    m_isPingCheckingEnabled = value;

    // Check 5 times
    if (m_isPingCheckingEnabled) {
        QTimer::singleShot(0, [&](){
            checkServersPing();
        });
        QTimer::singleShot(400, [&](){
            checkServersPing();
        });
        QTimer::singleShot(800, [&](){
            checkServersPing();
        });
        QTimer::singleShot(1200, [&](){
            checkServersPing();
        });
        QTimer::singleShot(1600, [&](){
            checkServersPing();
        });
    }
}

void Pinger::checkServersPing()
{
    if (!m_isPingCheckingEnabled) return;

    for (const QJsonValue &value : API_HTTP::Instance().getServersList()) {
        const QJsonObject &server = value.toObject();
        ping(server.value("ip").toString(), server.value("id").toString());
    }
}

double Pinger::getPing(const QString& serverId)
{
    return serversPing.value(serverId);
}

void Pinger::registerPingForServer(const QString& serverId, int ping)
{
    if (serversPing.contains(serverId)) {
        serversPing[serverId] = 0.9 * serversPing[serverId] + 0.1 * ping;
    }
    else {
        serversPing.insert(serverId, ping);
    }
}

QList<QPair<QString, double>> Pinger::getServersSortedByPing()
{
    qDebug() << "Pinger::getServersSortedByPing, serversPing size:" << serversPing.size();
    QList<QPair<QString, double>> servers;

    for (auto i = serversPing.begin(); i != serversPing.end(); ++i) {
        servers.append(qMakePair<QString, double>(i.key(), i.value()));
    }

    qSort(servers.begin(), servers.end(), [](QPair<QString, double> left, QPair<QString, double> right){
        return left.second < right.second;
    });

    return servers;
}


#ifdef Q_OS_WIN
void Pinger::ping(const QString& ip, const QString& server_id)
{
    //qDebug() << "Ping ip:" << ip;

    //HANDLE hIcmpFile;
    unsigned long ipaddr = inet_addr(ip.toLatin1());
    DWORD dwRetVal = 0;
    char SendData[] = "Data Buffer";
    DWORD ReplySize = 0;

    pong4 *pong = new pong4;
    pong->server_id = server_id;

    pong->hIcmpFile;
    pong->hIcmpFile = IcmpCreateFile();
    if (pong->hIcmpFile == INVALID_HANDLE_VALUE) {
        qDebug() << "Pinger :: Unable to open handle";
        qDebug() << "Pinger :: IcmpCreatefile returned error" << GetLastError();
        return;
    }

    // Allocate space for at a single reply
    ReplySize = sizeof (ICMP_ECHO_REPLY) + sizeof (SendData) + 8;
    pong->bufsize = ReplySize;


    pong->buf = (PICMP_ECHO_REPLY)malloc(ReplySize);
    if (pong->buf == NULL) {
        qDebug() << "Pinger :: Unable to allocate memory for reply buffer";
        return;
    }

    dwRetVal = IcmpSendEcho2(pong->hIcmpFile,
                             NULL,
                             icmpwin_callback_apc,
                             pong,
                             ipaddr,
                             SendData,
                             sizeof (SendData),
                             NULL,
                             pong->buf,
                             ReplySize,
                             1000);

    if (dwRetVal != 0) {
        goto out;
    }
    if (dwRetVal = GetLastError() != ERROR_IO_PENDING) {

        //qDebug() << QString("IcmpSendEcho2: error %1\n").arg(dwRetVal);
        switch (dwRetVal) {
        case ERROR_NETWORK_UNREACHABLE:
            break;
        case ERROR_HOST_UNREACHABLE:
            break;
        default:
            break;
        }
        goto out;
    }

    pong = NULL;                /* callback owns it now */
out:
    if (pong != NULL) {
        IcmpCloseHandle(pong->hIcmpFile);
        delete pong;
    }
}

VOID NTAPI icmpwin_callback_apc(PVOID ctx, PIO_STATUS_BLOCK iob, ULONG reserved)
{
    Q_UNUSED(iob);
    Q_UNUSED(reserved);

    pong4 *pong = (pong4 *)ctx;
    if (pong != NULL) {
        icmpwin_callback(pong);

        free (pong->buf);
        IcmpCloseHandle(pong->hIcmpFile);
        delete pong;
    }
}

VOID NTAPI icmpwin_callback(struct pong4 *pong)
{
    ICMP_ECHO_REPLY *reply;
    DWORD nreplies;

    nreplies = IcmpParseReplies(pong->buf, (DWORD)pong->bufsize);
    if (nreplies == 0) {
        Pinger::Instance().registerPingForServer(pong->server_id, 1000);

        DWORD error = GetLastError();
        if (error == IP_REQ_TIMED_OUT) {
            // DPRINTF2(("pong4: %p timed out\n", (void *)pong));
        }
        else {
            // DPRINTF(("pong4: %p: IcmpParseReplies: error %d\n",
            //          (void *)pong, error));
        }
        return;
    }

    reply = (ICMP_ECHO_REPLY *)pong->buf;

    if (reply->Options.OptionsSize != 0) {
        return;
    }


    if (reply->Status == IP_SUCCESS) {
        Pinger::Instance().registerPingForServer(pong->server_id, reply->RoundTripTime);
        //qDebug() << QDateTime::currentDateTime().toString() << ", Reply timeout:" << reply->RoundTripTime;
    }
    else {
        Pinger::Instance().registerPingForServer(pong->server_id, 1000);

        //u8_t type, code;

        switch (reply->Status) {
        case IP_DEST_NET_UNREACHABLE:
            break;
        case IP_DEST_HOST_UNREACHABLE:
            break;
        case IP_DEST_PROT_UNREACHABLE:
            break;
        case IP_PACKET_TOO_BIG:
            break;
        case IP_SOURCE_QUENCH:
            break;
        case IP_TTL_EXPIRED_TRANSIT:
            break;
        case IP_TTL_EXPIRED_REASSEM:
            break;
        default:
            //DPRINTF(("pong4: reply status %d, dropped\n", reply->Status));
            return;
        }


//        qDebug() << QString("pong4: reply status %1 -> type %2/code %3\n")
//                    .arg(reply->Status)
//                    .arg(type)
//                    .arg(code);
    }
}

#endif

#ifdef Q_OS_MAC
void Pinger::ping(const QString& ip, const QString& serverId)
{
    pingerObject.ping(ip, serverId);
}
#endif


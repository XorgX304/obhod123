#ifndef PINGER_H
#define PINGER_H

#include <QTimer>
#include <QString>
#include <QSettings>
#include <QHash>



#ifdef Q_OS_WIN
#include <WinSock2.h>  //includes Windows.h
#include <Ws2tcpip.h>

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID    Pointer;
  };
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/**
    typedef for PIO_APC_ROUTINE from MSDN
*/
typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );
//defined
#define PIO_APC_ROUTINE_DEFINED

#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>



#include <stdint.h>
typedef uint8_t u8_t ;

struct pong4 {
    size_t bufsize;
    PICMP_ECHO_REPLY buf;
    HANDLE hIcmpFile;
    QString server_id;
};

VOID NTAPI icmpwin_callback_apc(PVOID ctx, PIO_STATUS_BLOCK iob, ULONG reserved);
VOID NTAPI icmpwin_callback(struct pong4 *pong);

#endif


/**
 * @brief The Pinger class - General class for handling ping
 */
class Pinger : public QObject
{
    Q_OBJECT
public:
    static Pinger& Instance();

    bool isPingCheckingEnabled() const;
    void setPingCheckingEnabled(bool value);

    double getPing(const QString& serverId);
    void registerPingForServer(const QString& serverId, int ping);

    QList<QPair<QString, double> > getServersSortedByPing();


public slots:
    void checkServersPing();
    void startServersPing();
    void stopServersPing();

#ifdef Q_OS_MAC
protected slots:
    void pingFinished(const QString& ip, const QString& data, int delay);
#endif

private:
    Pinger();
    Pinger(Pinger const &) = delete;
    Pinger& operator= (Pinger const&) = delete;

    void ping(const QString& ip, const QString& serverId);

    QHash<QString, double> serversPing;   // <server_id, avg_ping>
    bool m_isPingCheckingEnabled;
    QTimer checkTimer;

#ifdef Q_OS_MAC
    PingerUnix pingerObject;
#endif
};

#endif // PINGER_H

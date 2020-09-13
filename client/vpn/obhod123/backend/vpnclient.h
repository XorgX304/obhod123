#ifndef _VPNCLIENT_H
#define _VPNCLIENT_H

#include <QThread>
#include <QProcess>
#include <QPointer>

#include "vpnutils.h"

#ifdef Q_OS_WIN
#include "../backend/tapcontroller_win.h"
#endif

class VPNClient;

class VPNAgent : public QThread
{
    Q_OBJECT

public:
    VPNAgent();
    void setClient(VPNClient* client);

protected:
    void  run();

private:
    VPNClient *m_vpnClient;
};

class VPNClient : public QThread
{
    Q_OBJECT

public:
    enum TYPE {INVALID = -1, OPENVPN = 0, PPTP, L2TP, SSTP, IKEV2, CAMOUFLAGE, TYPEEND};
    enum STATE {DISCONNECTED, DISCONNECTING, CONNECTING, CONNECTED, VPNERROR, VPNUNKNOWN};
    Q_ENUM(STATE)

    struct SETUP {
        TYPE    type;
        QString name;   // name of the interface, such as PPTP/L2TP/SSTP
        QString ip;     // ip of the remote server
        QString key;    // cert file path or private key

        SETUP() {
            type = INVALID;
        }
        SETUP(TYPE t, const QString& n = QString(), const QString& i = QString(), const QString& k = QString()) {
            type = t; name = n;
            key = k; ip = i;
        }
    };

    struct OPTION {
        TYPE    type;
        QString name;   // login user name
        QString pass;   // login password
        QString conn;   // SSTP, L2TP, PPTP, IKEv2 book name, OPENVPN config
        QString ext;    // extend command line, should be split by "|"
        SETUP   setup;  // contains the data used to setup

        OPTION() {
            type = INVALID;
        }
        OPTION(TYPE t, const QString& n, const QString& p, const QString& co, const QString& e = QString()) {
            type = t; name = n;
            pass = p; conn = co; ext = e;
        }
    };

    friend class VPNAgent;

public:
    VPNClient();
    ~VPNClient();

    bool open(OPTION);    // block the process but not the UI.
    bool close();           // block the process but not the UI.

    /**
     * @brief setup - Install TAP/TUN driver
     * @return true on the success
     */
    bool setup(SETUP);

    bool unsetup(SETUP);
    // remove all installed driver or interface.
    bool reset();

    STATE state();
    TYPE  type();

    // time out value for open/close/setup/unsetup.
    // when timeout, we will cancel current state and report error in message.
    void  setTimeout(quint32);

    // get byte send and recv of current connection process.
    void  getByteCount(quint64 &receive, quint64 &send);
    void  setByteCount(quint64 receive, quint64 send);

    static QString composeDir();

    void deleteNetworkPreference(TYPE type);

protected:
    void  run();
    void  setState(STATE);
    void  checker();

signals:
    // if error happens, we will change the state to error first then send error message.
    // in error state, the only thing we can do is to call close to reset the vpn.
    void message(QString);
    void stateChanged(VPNClient::STATE);

    // call state change also will trigger the follow signals.
    void connecting();
    void connected();
    void disconnected();
    void disconnecting();
    void vpnerror();
    void vpnunknown();

    void managementMessage(QString message);


public slots:
    void vpnunknownreceivedcallback();

private:
    QList<QByteArray> getAllNetworkServices() const;
    static void executeProgram(const QString& program);

    QPointer<QProcess>   m_compose;     // helper process
    OPTION      m_option;
    STATE       m_state;
    quint32     m_timeout;

    // vpn data details.
    quint64     m_snd, m_rcv;
    QPointer<VPNAgent>   m_agent;       // manager thread
    bool	    m_unknown_received;
    int 		m_try_count;
};

QString VPNCOMPOSE(VPNClient::TYPE protocol);

Q_DECLARE_METATYPE(VPNClient::STATE)

#endif  // _VPNCLIENT_H

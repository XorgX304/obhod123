#ifndef OBHOD123_H
#define OBHOD123_H

#include <QNetworkReply>
#include <QSystemTrayIcon>
#include <QAction>

#include "api_http.h"
#include "vpnclient.h"

#include "router.h"
#include "settings.h"

#ifdef Q_OS_WIN
#include "networkcontroller_win.h"
#endif

class Obhod123 : public QObject
{
    Q_OBJECT

public:
    explicit Obhod123(QWidget *parent = 0);
    ~Obhod123();

public:
    enum VPNStatus {
        VPNStatusInit,
        VPNStatusConnecting,
        VPNStatusConnected,
        VPNStatusDisconnecting,
        VPNStatusDisconnected,
        VPNStatusCanceling,
        VPNStatusError,
        VPNStatusCount
    };

    static QString VPNStatusString(enum VPNStatus m_vpnStatus);

    void reset();               // close the current connection and obhod123
    QByteArray pageDataByUrl(const QString& url);
    VPNClient::TYPE getTypeByIndex(int);   // deprecated
    VPNClient::TYPE getTypeByName(const QString& protocolName);
    int getIndexByType(VPNClient::TYPE) const; // returns -1 if error
    void setProtocolIndex(int index);

    bool onConnect(const QJsonObject& connectionParams);

    VPNClient  *getVpnClient() {return vpn;}

    bool autoConnectInProgress;


    QString vpnGate() const;
    void setVpnGate(const QString &vpnGate);

    VPNStatus vpnStatus() const;

public slots:
    bool onConnectAuto(const QJsonObject& connectionParams);
    void onClickDisconnect();
    void onDisconnect();
    void onCancel();
    void onMessage(const QString& message);
    void onStateChanged(VPNClient::STATE);
    //void onTimer();
    void onSetMTUSize();
    void onUpdateTray();
    void sslErrorsCallback (QNetworkReply * reply, const QList<QSslError> & errors );
    void networkAccessibilityChanged (QNetworkAccessManager::NetworkAccessibility accessible );
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
    void onNetworkError(QNetworkReply::NetworkError);
    void setVpnStatus(Obhod123::VPNStatus s);

signals:
    void autoTryingServer(QString serverId, int count);

    // transfered from VPNClient
    void vpnClientMessage(QString);
    void vpnClientStateChanged(VPNClient::STATE);

protected:


private:
    int  getPingByHostName(const QString& host, int, qreal& ping);

    QSystemTrayIcon m_tray;
    QMenu          *m_menu;
    QAction        *m_action;

    QTime          begin;
    QString        curip;

    VPNStatus      m_vpnStatus;

    int            m_ntimeout;
    QMenu         *m_recent;

    QString        m_vpngate, m_defgate, m_srvip;
    bool 		   m_network_error;
    bool cancelAutoConnect;

    QJsonObject m_connectionParams;  // comes from ui
    QMetaObject::Connection messagesConnection;

    VPNClient     *vpn;

};

#endif // OBHOD123_H

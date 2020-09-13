#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include <QObject>
#include <QString>
#include <QMap>

#define NetworkController_DEBUG

class NetworkController
{
public:
    static NetworkController& Instance();

//    void enableIPv6OnAllAdapters();
//    void disableIPv6OnAllAdapters();
//    void restoreIPv6OnAllAdapters();

    //static QMap<QString, bool> getNetworkAdapters(QString ipVersion);
    static QJsonArray getNetworkAdapters();

//    static QMap<QString, bool> getNetworkAdaptersIPv4() { return getNetworkAdapters("4"); }
//    static QMap<QString, bool> getNetworkAdaptersIPv6() { return getNetworkAdapters("6"); }

    static bool setDnsServersOnAdapter(const QString& adapterCLSID, const QString& dnsServerPrimary, const QString& dnsServerSecondary = "");
    static bool setDnsServersOnHardwareAdapters(const QString& dnsServerPrimary, const QString& dnsServerSecondary = "");

private:
    explicit NetworkController();
    NetworkController(NetworkController const &) = delete;
    NetworkController& operator= (NetworkController const&) = delete;

    //bool setEnableIPv6OnAdapter(const QString& adapterName, bool enable);


    static QString nvspBindPath();  // tool for managing network adapters

    QMap<QString, bool> originalIPv6AdaptersState;
    bool is_IPv6_disabled;
    bool is_IPv6_enabled;



};

#endif // NETWORKCONTROLLER_H

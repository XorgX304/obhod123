#include "networkcontroller_win.h"

#include <QProcess>
#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


#define TAP_EXE_ERROR { \
    qDebug() << "TapController: Can't start tapinstall.exe"; \
    return false; \
    }

#define TAP_NO_MATCHING_DEVICES_ERROR { \
    qDebug() << "TapController: No matching devices found"; \
    return false; \
    }

NetworkController &NetworkController::Instance()
{
    static NetworkController s;
    return s;
}

NetworkController::NetworkController() :
    is_IPv6_disabled(false),
    is_IPv6_enabled(false)
{
}


QString NetworkController::nvspBindPath()
{
#ifdef Q_OS_WIN
    if (QSysInfo::currentCpuArchitecture() == "i386") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x32\\nvspbind.exe";
    }
    else if (QSysInfo::currentCpuArchitecture() == "x86_64") {
        return qApp->applicationDirPath() + "\\openvpn\\drivers_x64\\nvspbind.exe";
    }
    else return "";
#endif
    return "";
}

QJsonArray NetworkController::getNetworkAdapters()
{
	//wmic nicconfig get Description,SettingID 
	// https://www.cs.cmu.edu/~tgp/scsadmins/winadmin/WMIC_Queries.txt
    QProcess nvspProc;
    nvspProc.start(nvspBindPath(), QStringList());
    if(!nvspProc.waitForStarted()) {
        qDebug() << "Can't start nvspbind.exe, path:" << nvspBindPath() << nvspProc.errorString();
        return QJsonArray();
    }

    nvspProc.waitForFinished();
    QString output = QString( nvspProc.readAll() );

    QJsonArray adapters;

    QRegularExpression re("({........-....-....-....-............})(\\r\\n)"
                          "(.+)(\\r\\n)"
                          "(.+)(\\r\\n)"
                          "(.+)(:\\r\\n)"
                          "((.+\\r\\n)+)");

    QRegularExpressionMatchIterator i = re.globalMatch(output);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        QJsonObject adapter;

        QString clsid = match.captured(1);

        QString device = match.captured(3);
        device.replace("\"", "");

        QString type = match.captured(5);
        type.replace("\"", "");

        QString name = match.captured(7);
        name.replace("\"", "");


        adapter.insert("clsid", clsid);
        adapter.insert("device", device);
        adapter.insert("type", type);
        adapter.insert("name", name);


        QString adapter_info = match.captured(9);
        adapter_info.replace("\r", "");

        QRegularExpression re1("(\\s+)(enabled|disabled)(:\\s+)(\\w+)(\\s+)(\\(.+\\))");
        QRegularExpressionMatchIterator i1 = re1.globalMatch(adapter_info);

        QJsonObject protocols;
        while (i1.hasNext()) {
            QRegularExpressionMatch match1 = i1.next();
            protocols.insert(match1.captured(4), match1.captured(2) == "enabled");
        }
        adapter.insert("protocols", protocols);

        adapters.append(adapter);
    }

//#ifdef NetworkController_DEBUG
//    qDebug().noquote() << "TapController: getNetworkAdapters:\n" << adapters;
//#endif

    return adapters;
}

bool NetworkController::setDnsServersOnAdapter(const QString &adapterCLSID, const QString &dnsServerPrimary, const QString &dnsServerSecondary)
{
    // wmic nicconfig where "(IPEnabled=TRUE) and (SettingID = '{6D981C26-7127-42EE-8A40-E5BE68A6D86E}')" call SetDNSServerSearchOrder ("8.8.8.8", "8.8.4.4")
    // error codes https://docs.microsoft.com/en-us/windows/desktop/cimwin32prov/setdnsserversearchorder-method-in-class-win32-networkadapterconfiguration

    QString args;
    if (dnsServerSecondary.isEmpty()) {
        args = QString("nicconfig where \"(IPEnabled=TRUE) and (SettingID = '%1')\" call SetDNSServerSearchOrder (\"%2\")")
                .arg(adapterCLSID)
                .arg(dnsServerPrimary);
    }
    else {
        args = QString("nicconfig where \"(IPEnabled=TRUE) and (SettingID = '%1')\" call SetDNSServerSearchOrder (\"%2\", \"%3\")")
                .arg(adapterCLSID)
                .arg(dnsServerPrimary)
                .arg(dnsServerSecondary);
    }



    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setNativeArguments(args);
    p.start("wmic.exe");
    p.waitForFinished(5000);

    QString result = QString(p.readAll());

    qDebug().noquote() << "NetworkController::setDnsServersOnAdapter :: args" << args;
    qDebug().noquote() << "NetworkController::setDnsServersOnAdapter :: result" << result;

    if (result.contains("ReturnValue = 0")
            || result.contains("No Instance(s) Available")) return true;
    else return false;
}

bool NetworkController::setDnsServersOnHardwareAdapters(const QString &dnsServerPrimary, const QString &dnsServerSecondary)
{
    const QJsonArray& adapters = getNetworkAdapters();

    bool result = true;

    for (const QJsonValue& value : adapters) {
        const QJsonObject adapter = value.toObject();
        const QString &device = adapter.value("device").toString();
        const QString &clsid = adapter.value("clsid").toString();
        const QString &name = adapter.value("name").toString();

        if (device == "tap0901"
                || device.startsWith("pci\\")
                || device.startsWith("usb\\")
                || device.startsWith("vmbus\\")
                || device.startsWith("ROOT\\VMS_MP\\")) {

            bool ok = setDnsServersOnAdapter(clsid, dnsServerPrimary, dnsServerSecondary);
            qDebug() << "NetworkController::setDnsServersOnHardwareAdapters :: Setting DNS for" << name
                     << ", result is" << ok;

            result &= ok;
        }
    }

    return result;
}


//QMap<QString, bool> NetworkController::getNetworkAdapters(QString ipVersion)
//{
//    QString magicString = "noAdapter";
//    if (ipVersion == "4") magicString = "ms_tcpip";
//    if (ipVersion == "6") magicString = "ms_tcpip6";


//    QProcess nvspProc;
//    nvspProc.start(nvspBindPath(), QStringList());
//    if(!nvspProc.waitForStarted()) {
//        qDebug() << "Can't start nvspbind.exe, path:" << nvspBindPath() << nvspProc.errorString();
//        return QMap<QString, bool>();
//    }

//    nvspProc.waitForFinished();
//    QString output = QString( nvspProc.readAll() );

//    QMap<QString, bool> adapters;

//    QRegularExpression re("({........-....-....-....-............})(\\r\\n)"
//                          "(.+\\r\\n)"
//                          "(.+\\r\\n)"
//                          "(.+:\\r\\n)"
//                          "((.+\\r\\n)+)");

//    QRegularExpressionMatchIterator i = re.globalMatch(output);

//    while (i.hasNext()) {
//        QRegularExpressionMatch match = i.next();
//        QString adapter_uid = match.captured(1);
//        QString adapter_info = match.captured(6);
//        adapter_info.replace("\r", "");
//        QStringList sl = adapter_info.split("\n", QString::SkipEmptyParts);
//        for (QString s : sl) {
//            if (!s.contains(magicString)) continue;
//            if (s.contains("enabled")) adapters.insert(adapter_uid, true);
//            else if (s.contains("disabled")) adapters.insert(adapter_uid, false);
//        }
//    }

//#ifdef IPv6_DEBUG
//    qDebug().noquote() << "TapController: getNetworkAdaptersIPv6:\n" << adapters;
//#endif

//    return adapters;
//}

//bool NetworkController::setEnableIPv6OnAdapter(const QString& adapterName, bool enable)
//{
//    QProcess nvspProc;
//    nvspProc.start(nvspBindPath(), QStringList() << (enable ? "/e" : "/d") << adapterName << "ms_tcpip6");
//    if(!nvspProc.waitForStarted()) {
//        qDebug() << "Can't start nvspbind.exe";
//        return false;
//    }

//    bool status = nvspProc.waitForFinished();
//    QString output = QString( nvspProc.readAll() );

//#ifdef IPv6_DEBUG
//    qDebug().noquote() << QString("TapController: Setting adapter status for %1 to '%2' finished with status '%3'").arg(adapterName).arg(enable).arg(status);
//#endif

//    return true;
//}

//void NetworkController::disableIPv6OnAllAdapters()
//{
//    if (! is_IPv6_disabled) {
//        // save original state only if ipv6 is not disabled already
//        originalIPv6AdaptersState = getNetworkAdaptersIPv6();
//    }

//    for (QString adapter : originalIPv6AdaptersState.keys()) {
//        setEnableIPv6OnAdapter(adapter, false);
//    }
//    is_IPv6_disabled = true;
//}

//void NetworkController::enableIPv6OnAllAdapters()
//{
//    if (! is_IPv6_enabled) {
//        // save original state only if ipv6 is not disabled already
//        originalIPv6AdaptersState = getNetworkAdaptersIPv6();
//    }

//    for (QString adapter : originalIPv6AdaptersState.keys()) {
//        setEnableIPv6OnAdapter(adapter, true);
//    }
//    is_IPv6_enabled = true;
//}

//void NetworkController::restoreIPv6OnAllAdapters()
//{
//    for (QString adapter : originalIPv6AdaptersState.keys()) {
//        if (originalIPv6AdaptersState.value(adapter)) setEnableIPv6OnAdapter(adapter,true);
//        else setEnableIPv6OnAdapter(adapter,false);
//    }
//    is_IPv6_disabled = false;
//    is_IPv6_enabled = false;
//}


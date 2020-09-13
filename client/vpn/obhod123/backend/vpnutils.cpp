#include <QApplication>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <QFile>

#include "vpnutils.h"
#include "vpndefine.h"

void Obhod123Utils::setMTUSize(const char *subname, int size)
{
    if(size == 0)
        return; // auto mode, do nothing.
#ifdef Q_OS_WIN
    if(qstrcmp(subname, VPNNAME) == 0)
        return;
    winhelperSetMTUSize(subname, size);
#endif

#ifdef Q_OS_MAC
    QProcess mtu;
    if(qstrcmp(subname, VPNPPTPNAME) == 0
            || qstrcmp(subname, VPNL2TPNAME) == 0) {
        mtu.start("/sbin/ifconfig", QStringList() << "ppp0" << "mtu" << QString::number(size));
    }
    if(qstrcmp(subname, VPNNAME) == 0) {
        // ifconfig tun0 mtu [mtusize]
        mtu.start("/sbin/ifconfig", QStringList() << "tun0" << "mtu" << QString::number(size));
    }
    mtu.waitForStarted();
    mtu.waitForFinished();
#endif
}

QString Obhod123Utils::saveConfigToTempFile(QString ovpn_file)
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + "adfkfiowDVNAWK.tmp";
    QFile file(filePath);
    bool ok = file.open(QIODevice::WriteOnly);
    if (!ok) qDebug() << "Failed to open adfkfiowDVNAWK.tmp";

    file.write(ovpn_file.toUtf8());

    file.close();

    return filePath;
}

QString Obhod123Utils::getOvpnGateway()
{
    QProcess ipconfig;
    ipconfig.start("ipconfig", QStringList() << "/all");
    ipconfig.waitForStarted();
    ipconfig.waitForFinished();

    QString d = ipconfig.readAll();
    d.replace("\r", "");
    //qDebug().noquote() << d;

    QStringList adapters = d.split(":\n");

    bool isTapV9Present = false;
    QString tapV9;
    for (int i = 0; i < adapters.size(); ++i) {
        if (adapters.at(i).contains("TAP-Windows Adapter V9")) {
            isTapV9Present = true;
            tapV9 = adapters.at(i);
            break;
        }
    }
    if (!isTapV9Present) return "";

    QStringList lines = tapV9.split("\n");
    for (int i = 0; i < lines.size(); ++i) {
        if (!lines.at(i).contains("DHCP")) continue;

        QRegularExpression re("(: )([\\d\\.]+)($)");
        QRegularExpressionMatch match = re.match(lines.at(i));

        if (match.hasMatch()) {
            qDebug().noquote() << "Current VPN Gateway IP Address: " << match.captured(0);
            return match.captured(2);
        }
        else continue;
    }

    return "";
}

QString Obhod123Utils::getCurrentVPNConfigIP()
{
    // Used for l2tp etc...
    QProcess ipconfig;
    ipconfig.start("ipconfig", QStringList() << "/all");
    ipconfig.waitForStarted();
    ipconfig.waitForFinished();

    QString d = ipconfig.readAll();
    qDebug().noquote() << d;
    d = d.mid(d.indexOf(VPNNAME));
    int b = d.indexOf("IP Address");
    if(b < 0)
        b = d.indexOf("IPv4 Address");
    QString sub = d.mid(b, d.indexOf("\n", b) - b);
    sub = sub.trimmed();
    sub = sub.mid(sub.lastIndexOf(":") + 1).trimmed();
    sub = sub.mid(0, sub.indexOf("("));
    qDebug().noquote() << "Current IP Address: " << sub;
    return sub;
}

QString Obhod123Utils::getDefaultGateway()
{
#ifdef Q_OS_WIN
    QProcess route;
    route.start("route", QStringList() << "print" << "0.0.0.0" << "mask" << "0.0.0.0");
    route.waitForStarted();
    if(route.state() != QProcess::NotRunning)
        route.waitForFinished();

    QByteArray data = route.readAll();
    data = data.mid(data.indexOf("0.0.0.0") + 8).trimmed();
    data = data.mid(data.indexOf("0.0.0.0") + 8).trimmed();
    if(data.isEmpty())
        return QString::null;
    return data.left(data.indexOf(" "));
#endif

#ifdef Q_OS_MAC
    {
        QProcess p;
        p.start("netstat -nr");
        p.waitForFinished();
        QByteArray data = p.readAll();
        QList<QByteArray> bList = data.split('\n');
        for(int index=0; index<bList.count();index++)
        {
            QString bLine = bList.at(index);
            if(bLine.contains("default",Qt::CaseInsensitive))
            {
                QStringList strList = bLine.split(" ");
                strList.removeDuplicates();
                for(int listIndex=0;listIndex<strList.count();listIndex++)
                {
                    QString str =  strList.at(listIndex);
                    if(str.split(".").count() == 4)
                    {
                        return str;
                    }
                }
            }
        }
    }
#endif

  // If nothing found on Windows or Mac
  return QString();
}

bool Obhod123Utils::checkIPFormat(const QString& ip)
{
    int count = ip.count(".");
    if(count != 3)
        return false;

    QStringList list = ip.trimmed().split(".");
    foreach(QString it, list) {
        if(it.toInt() <= 255 && it.toInt() >= 0)
            continue;
        return false;
    }
    return true;
}

QString Obhod123Utils::getIPAddress(const QString& host)
{
    //TODO rewrite to api calls
    qDebug().noquote() << "GetIPAddress: checking " + host;
    if(host.isEmpty()) {
        qDebug().noquote() << "GetIPAddress: host is empty.";
        return QString();
    }

    if(checkIPFormat(host)) {
        qDebug().noquote() << "GetIPAddress host is ip:" << host << host;
        return host;    // it is a ip address.
    }
    QProcess ping;

#ifdef Q_OS_MACX
    ping.start("ping", QStringList() << "-c1" << host);
#endif
#ifdef Q_OS_WIN
    ping.start("ping", QStringList() << QString("/n") << "1" << QString("/w") << "1" << host);
#endif
    ping.waitForStarted();

    QEventLoop loop;
    loop.connect(&ping, SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();

    QString d = ping.readAll();
    if(d.size() == 0)
        return QString();
    qDebug().noquote() << d;

    QString ip;
#ifdef Q_OS_MACX
    ip = getStringBetween(d, "(", ")");
#endif
#ifdef Q_OS_WIN
    ip = getStringBetween(d, "[", "]");
#endif
    qDebug().noquote() << "GetIPAddress:" << host << ip;
    return ip;
}

QString Obhod123Utils::getStringBetween(const QString& s, const QString& a, const QString& b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if(ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if(bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

QString Obhod123Utils::getIPFromOvpn(QString ovpn_file)
{
    QRegularExpression re("(remote )([\\d\\.]+)( )");
    QRegularExpressionMatch match = re.match(ovpn_file);

    if (match.hasMatch()) {
        return match.captured(2);
    }
    else return "";
}

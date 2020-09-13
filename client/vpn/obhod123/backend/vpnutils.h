#ifndef OBHOD123UTILS_H
#define OBHOD123UTILS_H

#include <QString>
#include <QByteArray>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpressionMatch>
#include <QStringList>


#ifdef Q_OS_WIN
#include "publib/winhelp.h"
#endif

/**
 * @brief The Obhod123Utils class - Useful functions used by the app, mostly by class Obhod123
 */
class Obhod123Utils
{

public:
    static QString getCurrentVPNConfigIP();
    static QString getOvpnGateway();
    static QString getIPAddress(const QString& host);
    static QString getStringBetween(const QString& s, const QString& a, const QString& b);
    static QString getDefaultGateway();
    static QString getIPFromOvpn(QString ovpn_file);
    static bool checkIPFormat(const QString& ip);
    static void setMTUSize(const char *subname, int size);
    static QString saveConfigToTempFile(QString ovpn_file);

};

#endif // OBHOD123UTILS_H

#include "entity.h"
#include "fuckmacro.h"


static QJsonObject *s_serversJson = 0;
static QJsonObject *s_usersJson = 0;
static QMutex *s_serversJson_mutex = 0;
static QMutex *s_usersJson_mutex = 0;

static QString *s_caPassword = 0;
static QHash<QString, QString> *s_staticRequests = 0;
static QJsonArray *s_connectionLog;


QJsonObject *serversJson()
{
    if (s_serversJson == NULL) {
        qDebug() << "Init s_serversJson";
        s_serversJson = new QJsonObject;
    }
    return s_serversJson;
}

QJsonObject *usersJson()
{
    if (s_usersJson == NULL) {
        qDebug() << "Init s_usersJson";
        s_usersJson = new QJsonObject;
    }
    return s_usersJson;
}

QMutex *serversJsonMutex()
{
    if (s_serversJson_mutex == NULL) {
        s_serversJson_mutex = new QMutex;
    }
    return s_serversJson_mutex;
}

QMutex *usersJsonMutex()
{
    if (s_usersJson_mutex == NULL) {
        s_usersJson_mutex = new QMutex;
    }
    return s_usersJson_mutex;
}

QString *caPassword()
{
    // implement to store it in registers (by QVarLengthArray???)
    if (s_caPassword == NULL) {
        s_caPassword = new QString(getRandomString(32));
    }
    return s_caPassword;
}

QHash<QString, QString> *staticRequests()
{
    if (s_staticRequests == NULL) {
        s_staticRequests = new QHash<QString, QString>;
    }
    return s_staticRequests;
}

void init()
{
    mlockall(MCL_CURRENT | MCL_FUTURE);

    qDebug() << "Init API, version" << API_VERSION_VALUE;

    qsrand(QTime::currentTime().msecsSinceStartOfDay());

    //QSettings s("/etc/obhod123.conf");

    loadServersJson();
    loadUsersJson();
    loadStaticRequests();

    // Every time reinit OVPN server
    ovpn_genconfig();
    ovpn_initpki();
    ovpn_run();
}

void ovpn_genconfig()
{
    QString server_ip = QProcessEnvironment::systemEnvironment().value("ip");
    qDebug() << "start ovpn_genconfig, server ip:" << server_ip;

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("bash", QStringList()
            << "/usr/local/bin/ovpn_genconfig"
            << "-u" << "udp://" + server_ip);


    p.waitForFinished();
    QString output = QString(p.readAll());
    qDebug().noquote() << "ovpn_genconfig:\n" << output;
}

void ovpn_initpki()
{
    qDebug() << "ovpn_initpki";

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("bash", QStringList()
            << "/usr/local/bin/ovpn_initpki");

    p.waitForStarted();

    // Wait for string "Enter New CA Key Passphrase:"
    {
        for (int i = 0; i < 1000; ++i) {
            bool isReadyRead = p.waitForReadyRead(100);
            if (isReadyRead) {
                QString output = QString(p.readAll());
                qDebug().noquote() << output;

                if (output.contains("Enter New CA Key Passphrase:")) {
                    p.waitForReadyRead(100); // just do nothing 100 msec
                    break;
                }
            }
        }
        qint64 bytesWritten = p.write(QString(*caPassword() + "\n").toUtf8());
        bool isWritten = p.waitForBytesWritten(10000);
        qDebug() << "Enter New CA Key Passphrase, written:" << bytesWritten << isWritten;
    }


    // Wait for string "Re-Enter New CA Key Passphrase:"
    {
        for (int i = 0; i < 1000; ++i) {
            bool isReadyRead = p.waitForReadyRead(100);
            if (isReadyRead) {
                QString output = QString(p.readAll());
                qDebug().noquote() << output;

                if (output.contains("Re-Enter New CA Key Passphrase:")) {
                    p.waitForReadyRead(100); // just do nothing 100 msec
                    break;
                }
            }
        }
        qint64 bytesWritten = p.write(QString(*caPassword() + "\n").toUtf8());
        bool isWritten = p.waitForBytesWritten(10000);
        qDebug() << "Re-Enter New CA Key Passphrase, written:" << bytesWritten << isWritten;
    }


    // Wait for string "[Easy-RSA CA]:"
    {
        for (int i = 0; i < 1000; ++i) {
            bool isReadyRead = p.waitForReadyRead(100);
            if (isReadyRead) {
                QString output = QString(p.readAll());
                qDebug().noquote() << output;

                if (output.contains("[Easy-RSA CA]:")) {
                    p.waitForReadyRead(100); // just do nothing 100 msec
                    break;
                }
            }
        }
        qint64 bytesWritten = p.write(QString("\n").toUtf8());
        bool isWritten = p.waitForBytesWritten(10000);
        qDebug() << "[Easy-RSA CA], written:" << bytesWritten << isWritten;
    }


    // Wait while ca init, string "Using configuration from /usr/share/easy-rsa/openssl-easyrsa.cnf"
    {
        //p.waitForFinished(120000); // since we have no eventloop, we are using this hack
        for (int i = 0; i < 300000; ++i) {
            bool isReadyRead = p.waitForReadyRead(1);
            if (isReadyRead) {
                QString output = QString(p.readAll());
                qDebug().noquote() << output;

                if (output.contains("Using configuration from /usr/share/easy-rsa/openssl-easyrsa.cnf")) {
                    p.waitForReadyRead(100); // just do nothing 100 msec
                    break;
                }
            }
        }
        qint64 bytesWritten = p.write(QString(*caPassword() + "\n").toUtf8());
        bool isWritten = p.waitForBytesWritten(10000);
        qDebug() << "Using configuration..., written:" << bytesWritten << isWritten;
    }


    // "Enter pass phrase for /etc/openvpn/pki/private/ca.key:""
    {
        for (int i = 0; i < 1000; ++i) {
            bool isReadyRead = p.waitForReadyRead(1000);
            if (isReadyRead) {
                QString output = QString(p.readAll());
                qDebug().noquote() << output;

                if (output.contains("Enter pass phrase for /etc/openvpn/pki/private/ca.key:")) {
                    p.waitForReadyRead(100); // just do nothing 100 msec
                    break;
                }
            }
        }
        qint64 bytesWritten = p.write(QString(*caPassword() + "\n").toUtf8());
        bool isWritten = p.waitForBytesWritten(10000);
        qDebug() << "Enter pass..., written:" << bytesWritten << isWritten;
    }

    p.waitForFinished();
    QString output = QString(p.readAll());
    qDebug().noquote() << "ovpn_initpki:\n" << output;
}

void ovpn_run()
{
    qDebug() << "ovpn_initpki";

    QProcess p;
    //p.setProcessChannelMode(QProcess::MergedChannels);

    p.startDetached("bash", QStringList()
            << "/usr/local/bin/ovpn_run");
}

QJsonArray *connectionLog()
{
    if (s_connectionLog == NULL) {
        s_connectionLog = new QJsonArray;
    }
    return s_connectionLog;
}



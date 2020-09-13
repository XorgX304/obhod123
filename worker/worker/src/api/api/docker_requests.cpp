#include "docker_requests.h"

bool d_addUser(QString user_id)
{
    // just add user to CA by name=random hash
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);


    // Debug local
//    p.start("docker", QStringList()
//            << "run"
//            << "-v" << OVPN_DATA + QString(":/etc/openvpn")
//            << "--rm"
//            << "-i"
//            << "obhod123/obhod123"
//            << "easyrsa"
//            << "build-client-full"
//            << user_id
//            << "nopass");

    // Production
    p.start("bash", QStringList()
            << "easyrsa"
            << "build-client-full"
            << user_id
            << "nopass");


    bool isStarted = p.waitForStarted(1000);
    //if (isStarted) qDebug() << "d_addUser P started";
    //else qDebug() << "d_addUser error" << p.errorString();

    if (!isStarted) return false;

    for (int i = 0; i < 10000; ++i) {
        bool isReadyRead = p.waitForReadyRead(10);
        if (isReadyRead) {
            QString output = QString(p.readAll());
            //qDebug().noquote() << "d_addUser output 1:" << output;

            if (output.contains("Request file already exists")) return false;
            if (output.contains("Enter pass phrase")) {
                p.waitForReadyRead(100); // just so nothing 100 msec
                break;
            }
        }
    }

    qint64 bytesWritten = p.write(QString(*caPassword() + "\n").toUtf8());
    bool isWritten = p.waitForBytesWritten(10000);
    //qDebug() << "d_addUser Is written:" << bytesWritten << isWritten;

    p.waitForReadyRead(10000);
    QString output = QString(p.readAll());
    //qDebug().noquote() << "d_addUser output 2:" << output;

    p.waitForFinished();
    //qDebug() << "d_addUser P finished";

    return true;
}

QString d_getUser(QString user_id)
{
    // just add user to CA by name=random hash
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    // Debug local
//    p.start("docker", QStringList()
//            << "run"
//            << "-v" << OVPN_DATA + QString(":/etc/openvpn")
//            << "--rm"
//            << "obhod123/obhod123"
//            << "ovpn_getclient"
//            << user_id );

    // Production
    p.start("bash", QStringList()
            << "ovpn_getclient"
            << user_id );

    bool isStarted = p.waitForStarted(1000);
    //if (isStarted) qDebug() << "d_getUser P started";
    //else qDebug() << "d_getUser error" << p.errorString();


    QString ovpnConfig;

    for (int i = 0; i < 10000; ++i) {
        bool isReadyRead = p.waitForReadyRead(10);
        if (isReadyRead) {
            QString output = QString(p.readAll());
            //qDebug().noquote() << "d_getUser output 1:" << output;

            if (output.contains("Unable to find")) {
                //qDebug() << "Unable to find found:" <<  output;
                return "";
            }
            if (output.contains("___END___")) {
                output.replace("___END___", "");
                ovpnConfig.append("\n" + output);
                break;
            }
            ovpnConfig.append("\n" + output);
        }
    }

    p.waitForFinished();
    return ovpnConfig;
}

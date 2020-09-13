#ifndef ENTITY_H
#define ENTITY_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QDebug>

#include <QObject>
#include <QString>

#include <sys/mman.h>


#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>
#include <QMutex>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QSettings>
#include <QTime>
#include <QProcess>

#include "nx_types.h"
#include "web_types.h"
#include "saveload.h"

#include "cryptofuck.h"


#define QT_DEBUG_PLUGINS 1

inline NxResponse RESPONSE_200(const QByteArray &ba) {
    NxResponse response;
    response.code = 200;

    QByteArray ba_encrypted = encryptByteArray(ba);

    //qDebug() << "RESPONSE_200:" << ba_encrypted;

    response.length = sizeof(char) * ba_encrypted.size();
    response.data = (char*)malloc(sizeof(char) * ba_encrypted.size() + 1);

    strcpy(response.data, ba_encrypted.data());
    return response;
}


inline NxResponse RESPONSE_200_PLAIN(const QByteArray &ba) {
    NxResponse response;
    response.code = 200;
    response.data = (char*)malloc(sizeof(char) * ba.size() + 1);
    response.length = sizeof(char) * ba.size();

    strcpy(response.data, ba.data());
    return response;
}

inline NxResponse RESPONSE_400() {
    return RESPONSE_200_PLAIN("\n");
}

inline NxResponse RESPONSE_404() {
    return RESPONSE_200_PLAIN("\n");
}

QHash<QString, QString> *staticRequests();
QJsonObject *serversJson();
QJsonObject *usersJson();

QMutex *serversJsonMutex();
QMutex *usersJsonMutex();


QString *caPassword();

QJsonArray *connectionLog();

void init();
void ovpn_genconfig();
void ovpn_initpki();
void ovpn_run();



#endif // ENTITY_H

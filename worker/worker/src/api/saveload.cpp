#include "saveload.h"

#include "entity.h"

#include <QFile>

void saveUsersJson()
{
    QMutexLocker mutexLocker(usersJsonMutex());

    QFile file_users(users_file_name);
    file_users.open(QIODevice::WriteOnly);

    file_users.write(QJsonDocument(*usersJson()).toJson());
    file_users.flush();
    file_users.close();

}

void saveServersJson()
{
    QMutexLocker mutexLocker(usersJsonMutex());

    QFile file_servers(servers_file_name);
    file_servers.open(QIODevice::WriteOnly);

    file_servers.write(QJsonDocument(*serversJson()).toJson());
    file_servers.flush();
    file_servers.close();
}



void loadUsersJson()
{
    QMutexLocker mutexLocker(usersJsonMutex());

    QFile file_users(users_file_name);
    file_users.open(QIODevice::ReadOnly);

    *(usersJson()) = QJsonDocument::fromJson(file_users.readAll()).object();
    file_users.close();

//    // append free user
//    QJsonObject trial;
//    trial.insert("id", "free");
//    trial.insert("valid", "2222222222");
//    trial.insert("mode", "free");


    qDebug().noquote() << QJsonDocument(*usersJson()).toJson();
}

void loadServersJson()
{
    QFile file_servers(servers_file_name);
    file_servers.open(QIODevice::ReadOnly);

    *(serversJson()) = QJsonDocument::fromJson(file_servers.readAll()).object();
    file_servers.close();

    qDebug().noquote() << QJsonDocument(*serversJson()).toJson();
}

void loadStaticRequests()
{
    QFile file_static_requests(static_file_name);
    file_static_requests.open(QIODevice::ReadOnly);

    QJsonArray a = QJsonDocument::fromJson(file_static_requests.readAll()).array();
    file_static_requests.close();

    for (const QJsonValue &v: a) {
        staticRequests()->insert(v.toObject().value("key").toString(), v.toObject().value("value").toString());
    }

    qDebug().noquote() << QJsonDocument(a).toJson();

}

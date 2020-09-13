#include "server_requests.h"


NxResponse s_getServers(WebRequest request)
{
    Q_UNUSED(request);
    return RESPONSE_200(QJsonDocument(*serversJson()).toJson());
}

NxResponse s_getServersLastRevision(WebRequest request)
{
    Q_UNUSED(request);
    int revision = serversJson()->value("revision").toInt();

    return RESPONSE_200(QString::number(revision).toUtf8());
}

NxResponse s_getUsers(WebRequest request)
{
    Q_UNUSED(request);
    return RESPONSE_200(QJsonDocument(*usersJson()).toJson());
}

NxResponse s_getUsersLastRevision(WebRequest request)
{
    QMutexLocker mutexLocker(usersJsonMutex());

    Q_UNUSED(request);
    int revision = usersJson()->value("revision").toInt();

    return RESPONSE_200(QString::number(revision).toUtf8());
}

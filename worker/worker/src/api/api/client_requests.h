#ifndef CLIENT_REQUESTS_H
#define CLIENT_REQUESTS_H

#include "web_types.h"
#include "entity.h"
#include "docker_requests.h"
#include "fuckmacro.h"

#include <QUrlQuery>
#include <QUrl>
#include <QCryptographicHash>

NxResponse c_test(WebRequest request);

NxResponse c_getServers(WebRequest request);
NxResponse c_getOvpnFile(WebRequest request);

NxResponse c_checkIfUserExists(WebRequest request);
NxResponse c_auth(WebRequest request);
NxResponse c_activateKey(WebRequest request);

//NxResponse c_newOrder_Fondy(WebRequest request);

//QString calculateFondyHash(const QJsonObject &o);


//NxResponse m_addUser(WebRequest request);


#endif // CLIENT_REQUESTS_H

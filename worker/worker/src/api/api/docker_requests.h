#ifndef DOCKER_REQUESTS_H
#define DOCKER_REQUESTS_H

#include "web_types.h"
#include "entity.h"

#include <QProcess>

#define OVPN_DATA "ovpn-data-example"
//#define OVPN_DATA "ovpn-data-example"


bool d_addUser(QString user_id);
// bool d_removeUser(QString user_id);  // no need as we will rebuild ca on every startup

QString d_getUser(QString user_id);

#endif // DOCKER_REQUESTS_H

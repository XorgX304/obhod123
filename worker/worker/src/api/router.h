#ifndef ROUTER_H
#define ROUTER_H


#include <QObject>
#include <QString>
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

#include "nx_types.h"
#include "web_types.h"

#include "entity.h"
#include "api/client_requests.h"
#include "api/management_requests.h"

#include "cryptofuck.h"



static int current_timestamp;
inline int get_current_timestamp()  { return current_timestamp;}
inline void set_current_timestamp(int value) { current_timestamp = value;}

// router
extern "C" NxResponse onNxRequest(NxRequest request);





#endif // ROUTER_H

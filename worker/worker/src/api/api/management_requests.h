#ifndef M_REQUESTS_H
#define M_REQUESTS_H

#include "web_types.h"
#include "entity.h"
#include "saveload.h"
#include "docker_requests.h"
#include "fuckmacro.h"

#include <QUrlQuery>
#include <QUrl>
#include <QCryptographicHash>

// Add here apis to manage server, for example reload configs, user databases, etc
NxResponse m_reloadJsons(WebRequest request);


#endif // M_REQUESTS_H

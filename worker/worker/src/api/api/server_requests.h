#ifndef SERVER_REQUESTS_H
#define SERVER_REQUESTS_H

#include "web_types.h"
#include "entity.h"


NxResponse s_getUsers(WebRequest request);
NxResponse s_getUsersLastRevision(WebRequest request);

NxResponse s_getServers(WebRequest request);
NxResponse s_getServersLastRevision(WebRequest request);

#endif // SERVER_REQUESTS_H

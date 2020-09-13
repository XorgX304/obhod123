#ifndef NX_INTERFACE_H
#define NX_INTERFACE_H


#include <time.h>
#include <sys/time.h>

#include "nx_types.h"
#include "/usr/local/include/nxweb/nxweb.h"

#include "fuckmacro.h"

NxResponse onNxRequest(NxRequest request);

static nxweb_result onRequest(
        nxweb_http_server_connection* conn,
        nxweb_http_request* req,
        nxweb_http_response* resp
        );


//NXWEB_DEFINE_HANDLER(api_v_1, .on_request=onRequest,
// .flags=NXWEB_PARSE_PARAMETERS|NXWEB_HANDLE_POST|NXWEB_HANDLE_GET| NXWEB_INWORKER);

NXWEB_DEFINE_HANDLER_PATCHED(.on_request=onRequest,
 .flags=NXWEB_PARSE_PARAMETERS|NXWEB_HANDLE_POST|NXWEB_HANDLE_GET| NXWEB_INPROCESS);




#endif // NX_INTERFACE_H

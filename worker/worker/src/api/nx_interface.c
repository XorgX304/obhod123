#include "nx_interface.h"
#include "nxweb/http_server.h"
#include <time.h>


nxweb_result onRequest(nxweb_http_server_connection *conn, nxweb_http_request *req, nxweb_http_response *resp)
{
//    printf("0x%" PRIXPTR "\n", conn->sock.fs.fd);


    //nxweb_set_response_content_type(resp, "application/json");
    //nxweb_add_response_header(resp, "Connection", "Keep-Alive");
    //nxweb_set_response_charset(resp, "utf8");


//    return NXWEB_OK;

    clock_t t = clock();

    static clock_t max_time = 0;
    static clock_t total_time = 0;
    static int req_count = 0;


    NxRequest request;

    request.uri = req->uri;
    request.data = req->content;
    if (req->get_method) request.method = 0;
    else if (req->post_method) request.method = 1;
    else request.method = 1;

    request.host = conn->remote_addr;
    printf("CONHost: %d", conn->remote_addr);


    NxResponse response = onNxRequest(request);


    nxweb_set_response_status(resp, response.code, 0);
    nxweb_set_response_content_type(resp, "text/html");
    //    nxweb_set_response_content_type(resp, "application/octet-stream");
//    nxweb_add_response_header(resp, "Server", "nginx/1.13.10");


    resp->keep_alive = 0;
    resp->chunked_encoding = 0;
    resp->chunked_autoencode = 0;
    resp->gzip_encoded = 0;
    resp->ssi_on = 0;
    resp->templates_on = 0;
    resp->no_cache = 1;

    if (response.code == 200 && response.data != NULL)  {
        nxweb_response_append_str(resp, response.data);
        free (response.data);
    }



    t = clock() - t;

//    req_count++;
//    if (req_count > 1) {
//        if (t > max_time) max_time = t;
//        total_time += t;
//        if (t > 0) {
//            printf( "%.0f , %d, ", (double)total_time/ (double)req_count, req_count);

//            printf( "%s:", req->uri);
//            printf( "%.0f\n", (double)t);
//        }
//    }





    return NXWEB_OK; // always return NXWEB_OK
}

#ifndef NX_TYPES_H
#define NX_TYPES_H

typedef struct NxRequest {
    int method;
    const char* uri;
    const char* data;
    char* host;
} NxRequest;

typedef struct NxResponse {
    int code;
    char* data;
    int length;
} NxResponse;




#endif // NX_TYPES_H

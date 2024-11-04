#ifndef REQUEST_H
#define REQUEST_H

#include "client_info.h"
#include <stdbool.h>

#define MAX_PATH_BYTES 1024

typedef enum RequestMethod {
    METHOD_POST,
    METHOD_GET,
} RequestMethod;

typedef struct Request {
    RequestMethod method;
    char path[MAX_PATH_BYTES];
} Request;

typedef enum ErrorEnum {
    ERR_MALFORMED_REQUEST,
} ErrorEnum;

typedef struct RequestOrError {
    bool has_error;
    union data {
        Request req;
        ErrorEnum err;
    } data;
} RequestOrError;

extern RequestOrError *create_request_or_error();
extern RequestOrError *parse_request(ClientInfo *client);
extern void free_request_or_error(RequestOrError *req_or_err);

#endif // REQUEST_H

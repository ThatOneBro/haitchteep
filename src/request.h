#ifndef REQUEST_H
#define REQUEST_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_INLINE_PATH_BYTES 64

typedef enum RequestMethod {
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_OPTIONS,
    METHOD_HEAD,
    METHOD_DELETE,
    METHOD_TRACE,
    METHOD_PATCH,
} RequestMethod;

typedef struct Request {
    bool has_external_path;
    union path {
        char inline_path[MAX_INLINE_PATH_BYTES];
        char *path_ptr;
    } path;
    char *body;
    size_t content_len;
    RequestMethod method;
    ContentType content_type;
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
extern void free_request_or_error(RequestOrError *req_or_err);

extern const char *VALID_METHODS_LITERALS[8];
extern const RequestMethod VALID_METHODS[8];

#endif // REQUEST_H

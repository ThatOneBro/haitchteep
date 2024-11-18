#include "request.h"
#include <stdlib.h>

const char *VALID_METHODS_LITERALS[] = { "GET", "POST", "PUT", "OPTIONS", "HEAD", "DELETE", "TRACE", "PATCH" };
const RequestMethod VALID_METHODS[] = { METHOD_GET, METHOD_POST, METHOD_PUT, METHOD_OPTIONS, METHOD_HEAD, METHOD_DELETE, METHOD_TRACE, METHOD_PATCH };

RequestOrError *create_request_or_error()
{
    RequestOrError *req_or_err = malloc(sizeof(RequestOrError));
    return req_or_err;
}

void free_request_or_error(RequestOrError *req_or_err)
{
    if (!req_or_err->has_error && req_or_err->data.req.has_external_path) {
        free(req_or_err->data.req.path.path_ptr);
    }
    free(req_or_err);
}

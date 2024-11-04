#include "request.h"
#include "client_info.h"
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

const char *VALID_METHODS_LITERALS[] = { "GET", "POST" };
const RequestMethod VALID_METHODS[] = { METHOD_GET, METHOD_POST };

RequestOrError *create_request_or_error()
{
    RequestOrError *req_or_err = malloc(sizeof(RequestOrError));
    return req_or_err;
}

void free_request_or_error(RequestOrError *req_or_err)
{
    free(req_or_err);
}

// TODO: Account for unexpected EOF

RequestOrError *parse_request(ClientInfo *client)
{
    assert(client->state == CLIENT_READY);
    size_t offset = 0;
    size_t curr = 0;
    size_t i;

    // Find the first word in the buffer
    while (client->buffer[curr] != ' ') {
        curr++;
    }

    RequestOrError *result = create_request_or_error();
    bool valid_method = false;

    // Compare to all possible valid methods
    for (i = 0; i < sizeof(VALID_METHODS_LITERALS) / sizeof(char *); i++) {
        if (memcmp(&client->buffer[offset], VALID_METHODS_LITERALS[i], curr) == 0) {
            result->data.req.method = VALID_METHODS[i];
            valid_method = true;
        }
    }

    // If we haven't created a request in the loop above,
    // That means that there was no valid method
    // We need to flag the request as malformed and return
    if (!valid_method) {
        result->has_error = true;
        result->data.err = ERR_MALFORMED_REQUEST;
        return result;
    }

    // If we made it here, we can continue parsing
    // First, reset curr and set offset to just after new space
    offset = ++curr;

    // Test to see if path is valid (starts with /)
    if (client->buffer[curr] != '/') {
        // This is a malformed request
        result->has_error = true;
        result->data.err = ERR_MALFORMED_REQUEST;
        return result;
    }

    while (client->buffer[curr] != ' ') {
        curr++;
    }

    memcpy(&result->data.req.path, &client->buffer[offset], curr - offset);
    result->data.req.path[curr - offset] = '\0';

    return result;
}

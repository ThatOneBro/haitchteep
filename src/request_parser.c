#include "client_info.h"
#include "request.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    if (curr - offset > MAX_INLINE_PATH_BYTES) {
        result->data.req.has_external_path = true;
        // Alloc a new buffer for path if it won't fit in the struct
        result->data.req.path.path_ptr = malloc(curr - offset);
        memcpy(result->data.req.path.path_ptr, &client->buffer[offset], curr - offset);
        result->data.req.path.path_ptr[curr - offset] = '\0';
    } else {
        memcpy(&result->data.req.path.inline_path, &client->buffer[offset], curr - offset);
        result->data.req.path.inline_path[curr - offset] = '\0';
    }

    // TODO Assert it's actually there first
    // Skip HTTP version
    curr += strlen(" HTTP/1.1\r\n");

    // Parse headers
    char *header_end;
    size_t content_length = 0;

    char header_name[64] = { 0 };
    char header_value[512] = { 0 };

    while (true) {
        header_end = strstr(&client->buffer[curr], "\r\n");
        if (!header_end || header_end == &client->buffer[curr]) {
            break; // End of headers
        }

        // Parse header name
        char *colon = strchr(&client->buffer[curr], ':');
        if (!colon || colon > header_end) {
            result->has_error = true;
            result->data.err = ERR_MALFORMED_REQUEST;
            return result;
        }

        size_t name_len = colon - &client->buffer[curr];
        size_t value_len = header_end - (colon + 2); // +2 to skip ": "

        memcpy(header_name, &client->buffer[curr], name_len);
        header_name[name_len] = '\0';
        memcpy(header_value, colon + 2, value_len);
        header_value[value_len] = '\0';

        // Check for special headers
        if (strcasecmp(header_name, "Content-Length") == 0) {
            content_length = atoi(header_value);
        } else if (strcasecmp(header_name, "Transfer-Encoding") == 0 && strcasecmp(header_value, "chunked") == 0) {
            // TODO: Support chunked encoding
            assert(1 != 1);
        }

        curr = header_end - client->buffer + 2; // +2 to skip \r\n
    }

    // Skip the final \r\n that separates headers from body
    curr += 2;

    if (content_length > 0) {
        // Content-Length header present
        result->data.req.body = malloc(content_length);
        memcpy(result->data.req.body, &client->buffer[curr], content_length);
        result->data.req.content_len = content_length;
    }

    return result;
}

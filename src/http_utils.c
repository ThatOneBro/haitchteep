#include "http_utils.h"
#include "request.h"
#include "response.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * Finds the first occurrence of CRLFCRLF in the buffer
 * @param buffer Pointer to the buffer to search
 * @param length Length of the buffer
 * @return Pointer to the start of CRLFCRLF or NULL if not found
 */
inline const char *find_headers_end(const char *buffer, size_t length)
{
    if (buffer == NULL || length < 4) {
        return NULL;
    }

    for (size_t i = 0; i <= length - 4; i++) {
        if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') {
            return &buffer[i];
        }
    }
    return NULL;
}

inline bool is_method_with_body(RequestMethod method)
{
    if (method == METHOD_POST || method == METHOD_PUT || method == METHOD_PATCH) {
        return true;
    }
    return false;
}

/**
 * Checks if an HTTP/1.1 request is complete
 * @param buffer Pointer to the buffer containing the request
 * @param length Length of the buffer
 * @return true if request is complete, false if more data needed
 */
inline bool check_http_end(ClientInfo *client)
{
    // Basic validation
    if (client->buffer == NULL || client->buf_used < 4) {
        return false;
    }

    char *buffer = client->buffer;
    size_t length = client->buf_used;

    if (client->req != NULL) {
        return false;
    }

    Request *req = client->req;

    if (!is_method_with_body(req->method) && (buffer[length - 4] == '\r' && buffer[length - 3] == '\n' && buffer[length - 2] == '\r' && buffer[length - 1] == '\n')) {
        return true;
    }

    // Find end of headers
    const char *headers_end = find_headers_end(buffer, length);
    if (!headers_end) {
        return false;
    }

    // Calculate body start and length
    const char *body_start = headers_end + 4;
    size_t headers_len = body_start - buffer;
    size_t body_len = length - headers_len;

    // Check Content-Length if present
    if (req->content_len != 0) {
        return body_len >= req->content_len;
    }

    // If no Content-Length or chunked encoding, assume request is complete
    // (this is technically not valid for HTTP/1.1 requests with bodies)
    return true;
}

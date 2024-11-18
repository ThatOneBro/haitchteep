#include "response.h"
#include "client_info.h"
#include "common.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

const char *CONTENT_TYPE_LITERALS[] = { "text/plain; charset=us-ascii", "application/json" };
const ContentType CONTENT_TYPES[] = { CONTENT_TYPE_PLAINTEXT, CONTENT_TYPE_JSON };

const char *STATUS_LITERALS[] = { "200 OK", "201 Created", "400 Bad Request", "404 Not Found" };
const HttpStatus STATUSES[] = { STATUS_OK, STATUS_CREATED, STATUS_BAD_REQUEST, STATUS_NOT_FOUND };

const char HTTP_VERSION[] = "HTTP/1.1";

Response *create_response()
{
    Response *res = malloc(sizeof(Response));
    return res;
}

void free_response(Response *res)
{
    if (res->content_body != NULL) {
        free(res->content_body);
    }
    free(res);
}

inline size_t add_header_to_buf(char *buf, size_t buf_size, size_t offset,
    const char *header_name, const char *header_val)
{
    int written = snprintf(buf + offset, buf_size - offset, "%s: %s\r\n",
        header_name, header_val);
    return (written > 0 && (size_t)written < buf_size - offset) ? (size_t)written : 0;
}

inline size_t marshal_response(char *buf, size_t buf_size, Response *res)
{
    if (!buf || !res || buf_size == 0) {
        return 0;
    }

    size_t pos = 0;
    int written;
    char temp_buf[128]; // For formatting numbers and dates

    // Write status line (HTTP version and status)
    const char *status_literal = NULL;
    for (size_t i = 0; i < sizeof(STATUSES) / sizeof(HttpStatus); i++) {
        if (STATUSES[i] == res->status) {
            status_literal = STATUS_LITERALS[i];
            break;
        }
    }
    if (!status_literal) {
        return 0; // Invalid status
    }

    written = snprintf(buf + pos, buf_size - pos, "%s %s\r\n",
        HTTP_VERSION, status_literal);
    if (written < 0 || (size_t)written >= buf_size - pos) {
        return 0;
    }
    pos += written;

    struct tm *tm_info = gmtime(&res->time.tv_sec);
    if (!tm_info) {
        return 0;
    }

    // Format RFC 2822 date
    strftime(temp_buf, sizeof(temp_buf),
        "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    written = add_header_to_buf(buf, buf_size, pos, "Date", temp_buf);
    if (!written)
        return 0;
    pos += written;

    // Add the Content-Type header if present
    const char *content_type_literal = NULL;
    for (size_t i = 0; i < sizeof(STATUSES) / sizeof(HttpStatus); i++) {
        if (CONTENT_TYPES[i] == res->content_type) {
            content_type_literal = CONTENT_TYPE_LITERALS[i];
            break;
        }
    }
    written = add_header_to_buf(buf, buf_size, pos,
        "Content-Type", content_type_literal);
    if (!written)
        return 0;
    pos += written;

    // Add the Content-Length header
    snprintf(temp_buf, sizeof(temp_buf), "%zu", res->content_len);
    written = add_header_to_buf(buf, buf_size, pos,
        "Content-Length", temp_buf);
    if (!written)
        return 0;
    pos += written;

    // End of headers
    written = snprintf(buf + pos, buf_size - pos, "\r\n");
    if (written < 0 || (size_t)written >= buf_size - pos) {
        return 0;
    }
    pos += written;

    // Add the body if present
    if (res->content_body && res->content_len > 0) {
        if (pos + res->content_len >= buf_size) {
            return 0; // Buffer too small
        }
        memcpy(buf + pos, res->content_body, res->content_len);
        pos += res->content_len;
    }

    return pos;
}

void write_response(ClientInfo *client, Response *res)
{
    char *buf = malloc(res->content_len + 1024); // TODO: Adjust this to be more reasonable for header sizes
    size_t res_len = marshal_response(buf, res->content_len + 1024, res);
    send(client->fd, buf, res_len, 0);
}

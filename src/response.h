#ifndef RESPONSE_H
#define RESPONSE_H

#include "client_info.h"
#include "common.h"
#include <time.h>

typedef enum HttpStatus {
    STATUS_OK,
    STATUS_CREATED,
    STATUS_BAD_REQUEST,
    STATUS_NOT_FOUND,
} HttpStatus;

typedef struct Response {
    struct timespec time;
    HttpStatus status;
    ContentType content_type;
    size_t content_len;
    char *content_body;
} Response;

extern Response *create_response();
extern void write_response(ClientInfo *client, Response *res);
extern void free_response(Response *res);

extern inline size_t add_header_to_buf(char *buf, size_t buf_size, size_t offset,
    const char *header_name, const char *header_val);
extern inline size_t marshal_response(char *buf, size_t buf_size, Response *res);

#endif // RESPONSE_H

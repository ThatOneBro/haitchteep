#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include "client_info.h"
#include <stdbool.h>
#include <stddef.h>

extern inline const char *find_headers_end(const char *buffer, size_t length);
extern inline bool is_method_with_body(RequestMethod method);
extern inline bool check_http_end(ClientInfo *client);

#endif // HTTP_UTILS_H

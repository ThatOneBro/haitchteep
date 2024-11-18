#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include "client_info.h"
#include "request.h"

extern RequestOrError *parse_request(ClientInfo *client);

#endif // REQUEST_PARSER_H

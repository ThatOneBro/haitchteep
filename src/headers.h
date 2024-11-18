#ifndef HEADERS_H
#define HEADERS_H

#define MAX_CONTENT_TYPE_LEN 30

typedef struct CommonHeaders {
    char content_type[MAX_CONTENT_TYPE_LEN];
} CommonHeaders;

#endif // HEADERS_H

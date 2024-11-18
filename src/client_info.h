#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include "request.h"
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef enum ClientState {
    CLIENT_WRITING,
    CLIENT_READY,
    CLIENT_DONE,
} ClientState;

// Structure to track client state
typedef struct ClientInfo {
    Request *req;
    int fd; // Socket file descriptor
    char *buffer; // Dynamic buffer for incomplete reads
    size_t buf_used; // Amount of buffer currently used
    size_t buf_size; // Total buffer size
    ClientState state;
} ClientInfo;

extern ClientInfo *create_client(int fd);
extern void free_client(ClientInfo *client);
extern void handle_client_data(ClientInfo *client);

#endif // CLIENT_INFO_H

#include "client_info.h"
#include "http_utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// Initialize a new client structure
ClientInfo *create_client(int fd)
{
    ClientInfo *client = malloc(sizeof(ClientInfo));
    if (!client)
        return NULL;

    client->fd = fd;
    client->buf_size = BUFFER_SIZE;
    client->buf_used = 0;
    client->buffer = malloc(BUFFER_SIZE);
    client->state = CLIENT_WRITING;

    if (!client->buffer) {
        free(client);
        return NULL;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return client;
}

// Clean up client structure
void free_client(ClientInfo *client)
{
    if (client) {
        if (client->buffer) {
            free(client->buffer);
        }
        close(client->fd);
        free(client);
    }
}

// Handle data from a client
void handle_client_data(ClientInfo *client)
{
    while (1) {
        // Ensure we have room in the buffer
        if (client->buf_used == client->buf_size) {
            size_t new_size = client->buf_size * 2;
            char *new_buf = realloc(client->buffer, new_size);
            if (!new_buf) {
                perror("realloc failed");
                break;
            }
            client->buffer = new_buf;
            client->buf_size = new_size;
        }

        // TODO: Convert this to recv, potentially more performant
        // Read what we can
        ssize_t bytes_read = read(client->fd, client->buffer + client->buf_used,
            client->buf_size - client->buf_used);

        if (bytes_read > 0) {
            client->buf_used += bytes_read;
            // Process the data here. For this example, we'll just print it.
            printf("Received %zd bytes: %.*s", bytes_read, (int)bytes_read,
                client->buffer + client->buf_used - bytes_read);

            // If the end of the HTTP request, set client state to ready
            if (check_http_end(client->buffer, client->buf_used)) {
                client->state = CLIENT_READY;
            }
        } else if (bytes_read == 0) {
            // End of stream
            // Set client state to ready
            client->state = CLIENT_READY;

            printf("Client closed connection. Total bytes received: %zu\n",
                client->buf_used);
            return;

        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No more data available right now
            return;

        } else {
            // Error occurred
            perror("read failed");
            shutdown(client->fd, SHUT_WR);
            return;
        }
    }
}

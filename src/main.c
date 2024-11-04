#include "client_info.h"
#include "request.h"
#include "response.h"
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 5
#define MAX_CLIENTS 10

const char INDEX_PATH[] = "/";

static char BAD_REQUEST_BODY[] = "Bad Request";
static Response BAD_REQUEST_RES = {
    .content_len = sizeof(BAD_REQUEST_BODY) - 1,
    .content_body = BAD_REQUEST_BODY,
    .content_type = CONTENT_TYPE_PLAINTEXT,
    .status = STATUS_BAD_REQUEST,
};

static char NOT_FOUND_BODY[] = "Not Found";
static Response NOT_FOUND_RES = {
    .content_len = sizeof(NOT_FOUND_BODY) - 1,
    .content_body = NOT_FOUND_BODY,
    .content_type = CONTENT_TYPE_PLAINTEXT,
    .status = STATUS_NOT_FOUND,
};

static char DEFAULT_RES_ROOT_BODY[] = "Hello, World!";
static Response DEFAULT_RES_ROOT = {
    .content_len = sizeof(DEFAULT_RES_ROOT_BODY) - 1,
    .content_body = DEFAULT_RES_ROOT_BODY,
    .content_type = CONTENT_TYPE_PLAINTEXT,
    .status = STATUS_OK,
};

void handle_http_request(ClientInfo *client, RequestOrError *req_or_err)
{
    Response res;

    if (req_or_err->has_error) {
        // Handle parsing error
        switch (req_or_err->data.err) {
        case ERR_MALFORMED_REQUEST:
            // Copy bad request res
            res = BAD_REQUEST_RES;
            break;
        }
    } else {
        Request *req = &req_or_err->data.req;

        if (req->method == METHOD_GET && strcmp(req->path, INDEX_PATH) == 0) {
            res = DEFAULT_RES_ROOT;
        } else {
            // Respond with 404
            res = NOT_FOUND_RES;
        }
    }

    // Timestamp request before writing
    clock_gettime(CLOCK_REALTIME, &res.time);
    // Write response to socket
    write_response(client, &res);
    // Signal we are done with client
    client->state = CLIENT_DONE;
}

int main()
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Set up polling
    struct pollfd fds[MAX_CLIENTS + 1]; // +1 for the server socket
    ClientInfo *clients[MAX_CLIENTS] = { NULL };
    int nfds = 1;

    // Initialize server fd
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        int poll_result = poll(fds, nfds, -1);

        if (poll_result < 0) {
            if (errno == EINTR)
                continue;
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        // Check server socket for new connections
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

            if (client_fd < 0) {
                perror("accept failed");
                continue;
            }

            // Find a free slot
            int slot;
            for (slot = 0; slot < MAX_CLIENTS; slot++) {
                if (clients[slot] == NULL)
                    break;
            }

            if (slot >= MAX_CLIENTS) {
                printf("Too many clients. Rejecting connection.\n");
                close(client_fd);
                continue;
            }

            // Create new client structure
            clients[slot] = create_client(client_fd);
            if (!clients[slot]) {
                perror("Failed to create client structure");
                close(client_fd);
                continue;
            }

            // Add to poll set
            fds[nfds].fd = client_fd;
            fds[nfds].events = POLLIN;
            nfds++;

            printf("New connection accepted on fd %d\n", client_fd);
        }

        // Check client sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP)) {
                int client_idx = i - 1;
                handle_client_data(clients[client_idx]);

                printf("State: %d\n", clients[client_idx]->state);

                switch (clients[client_idx]->state) {
                case CLIENT_WRITING:
                    printf("writing\n");
                    continue;
                case CLIENT_READY: {
                    printf("Hello\n");
                    RequestOrError *req_or_err = parse_request(clients[client_idx]);
                    handle_http_request(clients[client_idx], req_or_err);
                    free_request_or_error(req_or_err);
                }
                }

                // If handle_client_data reached end of stream or encountered error
                if (clients[client_idx]->state == CLIENT_DONE || clients[client_idx]->buf_used == 0 || errno != 0) {
                    // Signal that we're done sending
                    shutdown(clients[client_idx]->fd, SHUT_WR);

                    // Free the client
                    free_client(clients[client_idx]);
                    clients[client_idx] = NULL;

                    // Remove from poll set
                    if (i < nfds - 1) {
                        fds[i] = fds[nfds - 1];
                        clients[client_idx] = clients[nfds - 2];
                    }
                    nfds--;
                    i--; // Recheck this slot since we moved another fd here
                }
            }
        }
    }

    // Cleanup
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            free_client(clients[i]);
        }
    }
    close(server_fd);
    return 0;
}

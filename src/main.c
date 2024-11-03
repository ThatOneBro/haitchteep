#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 5
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Structure to track client state
typedef struct ClientInfo {
  int fd;          // Socket file descriptor
  char *buffer;    // Dynamic buffer for incomplete reads
  size_t buf_used; // Amount of buffer currently used
  size_t buf_size; // Total buffer size
} ClientInfo;

// Initialize a new client structure
ClientInfo *create_client(int fd) {
  ClientInfo *client = malloc(sizeof(ClientInfo));
  if (!client)
    return NULL;

  client->fd = fd;
  client->buf_size = BUFFER_SIZE;
  client->buf_used = 0;
  client->buffer = malloc(BUFFER_SIZE);

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
void free_client(ClientInfo *client) {
  if (client) {
    if (client->buffer) {
      free(client->buffer);
    }
    close(client->fd);
    free(client);
  }
}

// Handle data from a client
void handle_client_data(ClientInfo *client) {
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

    } else if (bytes_read == 0) {
      // End of stream
      printf("Client closed connection. Total bytes received: %zu\n",
             client->buf_used);
      // Signal that we're done sending
      shutdown(client->fd, SHUT_WR);
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

int main() {
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
  ClientInfo *clients[MAX_CLIENTS] = {NULL};
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
      int client_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

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

        // If handle_client_data reached end of stream or encountered error
        if (clients[client_idx]->buf_used == 0 || errno != 0) {
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

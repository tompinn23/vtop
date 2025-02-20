#pragma once

#include <stddef.h>

enum client_state {
    CLIENT_NEW,
    CLIENT_HANDSHAKE,
};

typedef struct client {
    int fd;
    enum client_state state;

    int wpending, rpending;
} client_t;

typedef struct server {
    int listenfd;
    client_t *clients;
    size_t nclients, maxclients;
} server_t;

server_t *server_new();
int server_bind(server_t *sv, const char *addr);
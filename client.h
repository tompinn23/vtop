#pragma once

#include "util.h"

enum client_status {
    CLIENT_OK,
    CLIENT_READ,
    CLIENT_WRITE,
};

enum client_state {
    CLIENT_STATE_NEW,
    CLIENT_STATE_RECV_HELLO,
    CLIENT_STATE_SEND_HELLO,
    CLIENT_STATE_ERROR,
};

typedef struct client {
    int socket;
    int state;
    int inmsg;

    char *buff;
    size_t bufflen;
    size_t rpos;

    int errcode;
    char *errmsg;

    fdbuf_t *buf;
} client_t;

client_t *client_new(int socket);
int client_handshake(client_t *c);
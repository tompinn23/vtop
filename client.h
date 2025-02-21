#pragma once

struct iobuf;

struct client {
    int state;
    int errcode;
    const char *errmsg;
};

enum {
    CLIENT_READ,
    CLIENT_WRITE
};

struct client *client_new();

int client_handshake(struct client *c, struct iobuf *iobuf);

int client_process(struct client *c, struct iobuf *io, int event);
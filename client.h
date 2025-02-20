#pragma once

struct iobuf;

struct client {
    int state;
};

enum {
    CLIENT_READ,
    CLIENT_WRITE
};

struct client *client_new();

int client_process(struct client *c, struct iobuf *io, int event);
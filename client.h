#pragma once

struct client {
    int fd;
    int state;
};

enum {
    CLIENT_READ,
    CLIENT_WRITE
};
#pragma once

#include <stddef.h>

enum xev_backend {
    XEV_BACKEND_POLL = (1 << 0),
    XEV_BACKEND_EPOLL = (1 << 1),
    XEV_BACKEND_URING = (1 << 2),
    XEV_BACKEND_IOCP = (1 << 3),
};

enum xev_flags {
    XEV_THREADPOOL = (1 << 0),
    XEV_QUEUE_GROW = (1 << 1),
};

typedef struct xev_evt {
    void* data;
    int (*cb)(struct xev_evt* ev, void *data);
} xev_evt_t;

typedef struct xev_fd_evt {
    xev_evt_t evt;
    int fd;
    int events;
} xev_fd_evt_t;

typedef struct xev_loop {
    int backend;
    int flags;

    xev_evt_t *queued;
    size_t qlen;
    size_t qcap;

    void* data;
    int (*tick)(struct xev_loop* loop, long timeout);
    int (*submit)(struct xev_loop* loop, xev_evt_t* ev);
} xev_t;


xev_t *xev_init(int backends, int flags);


int xev_submit(xev_t* loop, xev_evt_t ev);


xev_fd_evt_t xev_fd_evt(int fd, int events, int (*cb)(xev_evt_t* ev, void *data));
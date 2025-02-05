#include "xev.h"

#include <stdlib.h>

#define X(x) extern int __xev_##x##_init(xev_t* loop);
#include "xev.backends.inc"
#undef X


int select_backend(int backends) {
#define X(x)    if(backends & XEV_BACKEND_##x) { return XEV_BACKEND_##x; }
#include "xev.backends.inc"
#undef X
    return -1;
}

xev_t *xev_init(int backends, int flags) {
    xev_t *loop;
    int backend;
    
    backend = select_backend(backends);
    if(backend < 0) {
        return NULL;
    }

    loop = malloc(sizeof(xev_t));
    if(!loop) {
        return NULL;
    }

    loop->backend = backend;
    loop->flags = flags;
    loop->queued = calloc(8, sizeof(xev_evt_t));
    loop->qlen = 0;
    loop->qcap = 8;
    loop->data = NULL;

#define X(x)    if(backend == XEV_BACKEND_##x) { __xev_##x##_init(loop); }
#include "xev.backends.inc"
#undef X

    return loop;
}

int xev_submit(xev_t* loop, xev_evt_t ev) {
    size_t newcap;
    xev_evt_t *new;

    if(loop->qlen == loop->qcap) {
        if(loop->flags & XEV_QUEUE_GROW) {
            newcap = loop->qcap * 2;
            new = realloc(loop->queued, loop->qcap * sizeof(xev_evt_t));
            if(!new) {
                return -1;
            }
            loop->queued = new;
            loop->qcap = newcap;
        } else {
            return 0;
        }
    }

    loop->queued[loop->qlen++] = ev;
    return 1;
}


xev_fd_evt_t xev_fd_evt(int fd, int events, int (*cb)(xev_evt_t* ev, void *data)) {

}


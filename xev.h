
enum xev_backend {
    XEV_BACKEND_POLL = (1 << 0),
    XEV_BACKEND_EPOLL = (1 << 1),
    XEV_BACKEND_URING = (1 << 2),
    XEV_BACKEND_IOCP = (1 << 3),
};

enum xev_flags {
    XEV_THREADPOOL = (1 << 0),
};

typedef struct xev_evt {

} xev_evt_t;

typedef struct xev_loop {
} xev_t;


xev_t *xev_init(int backends, int flags);

int xev_oneshot(xev_t* loop, xev_evt_t* ev);
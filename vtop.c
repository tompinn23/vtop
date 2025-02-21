#define _GNU_SOURCE
#include "vtop.h"

#include "log.h"
#include "util.h"
#include "proto.h"
#include "client.h"

#include <stdlib.h>
#include <sys/poll.h>
#include <signal.h>
#include <assert.h>
#include <sys/socket.h>

volatile sig_atomic_t hup = 0;
volatile sig_atomic_t child = 0;
volatile sig_atomic_t term = 0;

#define QEMU_STDOUT 0
#define QEMU_QMP 1
#define API_CLIENT 2

struct fd_data {
    int type;
    iobuf_t *iobuf;
    union {
      struct client *client;
    };
};

typedef struct loop {
    int fd;

    struct pollfd *pfds;
    size_t npfds;
    size_t pfdsz;

    struct fd_data *fdmap;
    size_t fdmapsz;

    int accepting, iterating;
} loop;


static void sighandleterm(int sig) {
    term = 1;
}

static void sighandlechild(int sig) {
    child = 1;
}

static void fdmap_resize(loop *loop, int fd) {
    struct fd_data *f;

    if(fd < loop->fdmapsz) {
        return;
    }

    f = realloc(loop->fdmap, fd + 1);
    if(f == NULL) {
        abort();
    }

    loop->fdmap = f;
    loop->fdmapsz = fd + 1;
}

static void fdmap_setclient(loop *loop, int fd, struct iobuf *iobuf, struct client *c) {
  fdmap_resize(loop, fd);
  struct fd_data *data = &loop->fdmap[fd];
  data->type = API_CLIENT;
  data->iobuf = iobuf;
}

/* Allocate or dynamically resize our poll fds array.  */
static void pollfds_maybe_resize(loop* loop) {
  size_t i;
  size_t n;
  struct pollfd* p;

  if (loop->npfds < loop->pfdsz)
    return;

  n = loop->pfdsz ? loop->pfdsz * 2 : 64;
  p = realloc(loop->pfds, n * sizeof(*loop->pfds));
  if (p == NULL)
    abort();

  loop->pfds = p;
  for (i = loop->pfdsz; i < n; i++) {
    loop->pfds[i].fd = -1;
    loop->pfds[i].events = 0;
    loop->pfds[i].revents = 0;
  }
  loop->pfdsz = n;
}

/* Primitive swap operation on poll fds array elements.  */
static void pollfds_swap(loop* loop, size_t l, size_t r) {
  struct pollfd pfd;
  pfd = loop->pfds[l];
  loop->pfds[l] = loop->pfds[r];
  loop->pfds[r] = pfd;
}

/* Add a watcher's fd to our poll fds array with its pending events.  */
static void pollfds_add(loop* loop, int fd, int events) {
  size_t i;
  struct pollfd* pe;

  /* If the fd is already in the set just update its events.  */
  assert(!loop->iterating);
  for (i = 0; i < loop->npfds; ++i) {
    if (loop->pfds[i].fd == fd) {
      loop->pfds[i].events = events;
      return;
    }
  }

  /* Otherwise, allocate a new slot in the set for the fd.  */
  pollfds_maybe_resize(loop);
  pe = &loop->pfds[loop->npfds++];
  pe->fd = fd;
  pe->events = events;
}

/* Remove a watcher's fd from our poll fds array.  */
static void pollfds_del(loop* loop, int fd) {
  size_t i;
  assert(!loop->iterating);
  for (i = 0; i < loop->npfds;) {
    if (loop->pfds[i].fd == fd) {
      /* swap to last position and remove */
      --loop->npfds;
      pollfds_swap(loop, i, loop->npfds);
      loop->pfds[loop->npfds].fd = -1;
      loop->pfds[loop->npfds].events = 0;
      loop->pfds[loop->npfds].revents = 0;
      /* This method is called with an fd of -1 to purge the invalidated fds,
       * so we may possibly have multiples to remove.
       */
      if (-1 != fd)
        return;
    } else {
      /* We must only increment the loop counter when the fds do not match.
       * Otherwise, when we are purging an invalidated fd, the value just
       * swapped here from the previous end of the array will be skipped.
       */
       ++i;
    }
  }
}


int run_loop(int sv) {
    struct pollfd *pfds;
    size_t npfds;
    struct loop *loop;
    sigset_t sigset, pollset;
    struct sigaction sa;
    int ret, rc;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sa.sa_handler = sighandleterm;
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
      un_log_errno(LOG_ERR, "signal: set TERM");
      goto out;
    }
    sa.sa_handler = sighandlechild;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
      un_log_errno(LOG_ERR, "signal: set CHILD");
      goto out;
    }
  
    sigemptyset(&pollset);
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sigset, NULL);


init:
    term = child = 0;
    loop = calloc(1, sizeof(*loop));
    loop->accepting = 1;

    pollfds_add(loop, sv, POLLIN);

    struct timespec timeout = {
        .tv_sec = 1,
        .tv_nsec = 0,
    };

pollagain:
    pfds = loop->accepting ? loop->pfds : loop->pfds + 1;
    npfds = loop->accepting ? loop->npfds : loop->npfds - 1;

    un_log(LOG_DEBUG, "polling");
    ret = ppoll(pfds, npfds, &timeout, &pollset); 
    if(ret < 0 && errno != EINTR) {
        un_log_errno(LOG_ERR, "poll failure");
    }
    un_log(LOG_DEBUG, "poll returned %d, term: %d child: %d", ret, term, child);

    if(term) {
        un_log(LOG_INFO, "exiting C-c");
        goto out;
    }
    for(size_t i = 0; i < npfds && ret > 0; i++) {

        if(!loop->accepting) {
            un_log(LOG_DEBUG, "not accepting: polling");
            goto pollagain;
        }
        if(pfds[0].revents & POLLHUP) {
            un_log(LOG_ERR, "poll: socket disconnect");
            goto out;
        } else if(pfds[0].revents & POLLERR) {
            un_log(LOG_ERR, "poll: socket error");
        } else if(!(pfds[0].revents & POLLIN)) {
            goto pollagain;
        }

        int fd = accept(sv, NULL, NULL);
        if(fd < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                goto pollagain;
            }
            un_log_errno(LOG_ERR, "accept: new connection");
            goto out;
        }
        struct iobuf *buf = iobuf_createfd(8192, fd);
        struct client *client = client_new();
        fdmap_setclient(loop, fd, buf, client);
        rc = client_handshake(client, buf);
        if(rc == CLIENT_READ) {
          pollfds_add(loop, fd, POLLIN);
        } else if(rc == CLIENT_WRITE) {
          pollfds_add(loop, fd, POLLOUT);
        }
    }

    goto pollagain;




out:
    return -1;
}
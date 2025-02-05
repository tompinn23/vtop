#define _GNU_SOURCE
#include "qemu.h"

#include "log.h"
#include "util.h"

#include <poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

static int xsockpair(int *sock) {
    int ret;
    sock[0] = sock[1] = -1;
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);
    if(ret < 0) {
        un_log_errno(LOG_ERR, "failed to create socketpair");
        return 0;
    }
    // int flags;
    // if((flags = fcntl(sock[0], F_GETFL, 0)) < 0) {
    //     un_log_errno(LOG_ERR, "failed to get socket flags");
    //     return 0;
    // } else if(fcntl(sock[0], F_SETFL, flags | O_NONBLOCK) < 0) {
    //     un_log_errno(LOG_ERR, "failed to set nonblock flags");
    //     return 0;
    // }
  return 1;
}

qemu_t *qemu_new(void) {
    qemu_t *q = malloc(sizeof(qemu_t));
    if(q == NULL) {
        un_log_errno(LOG_ERR, "failed to allocate memory for qemu_t");
        return NULL;
    }
    q->sock = -1;
    q->preconfig = true;
    return q;
}


static void timespec_sub(struct timespec *r, const struct timespec *a,
		const struct timespec *b) {
	r->tv_sec = a->tv_sec - b->tv_sec;
	r->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (r->tv_nsec < 0) {
		r->tv_sec--;
		r->tv_nsec += 1000000000;
	}
}

int qemu_terminate(qemu_t *q) {
    int ret = 0;
    int status;
    struct pollfd pfd = { .fd = q->sock, .events = POLLIN };
    sigset_t sigmask, oldmask;
    struct timespec timeout = {.tv_nsec = 0, .tv_sec = 1};

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &sigmask, &oldmask) < 0) {
        un_log_errno(LOG_ERR, "failed to block SIGCHLD");
        return -errno;
    }

    if(q->pid > 0) {
        if(!q->sigterm) {
            q->sigterm = true;
            un_log(LOG_DEBUG, "terminating qemu");
            if((ret = kill(q->pid, SIGTERM)) < 0) {
                un_log_errno(LOG_ERR, "failed to SIGTERM qemu");
                ret = -errno;
                goto killkill;
            }
        }
        for(;;) {
            ret = sigtimedwait(&sigmask, NULL, &timeout);
            if(ret < 0) {
                if(errno != EINTR && errno != EAGAIN) {
                    ret = -errno;
                    un_log_errno(LOG_ERR, "failed to wait for signal");
                    goto killkill;
                }
            }
            un_log(LOG_DEBUG, "waiting on %d", q->pid);
            if((ret = waitpid(q->pid, &status, WNOHANG)) < 0) {
                if(errno != EINTR) {
                    un_log_errno(LOG_ERR, "failed to wait for qemu");
                    ret = -errno;
                    goto killkill;
                }
            }
            if(ret > 0 && WIFEXITED(status)) {
                q->pid = 0;
                ret = 1;
                break;
            }
            clock_gettime(CLOCK_MONOTONIC, &now);
            timespec_sub(&now, &now, &start);
            if(now.tv_sec >= 30) {
                un_log(LOG_ERR, "qemu took too long to terminate, sending SIGKILL");
                goto killkill;
            }
        }
    }
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        un_log_errno(LOG_ERR, "failed to unblock SIGCHLD");
        return -errno;
    }
    return ret;
killkill:
    un_log(LOG_DEBUG, "sending SIGKILL to qemu");
    if ((ret = kill(q->pid, SIGKILL)) < 0) {
        un_log_errno(LOG_ERR, "failed to SIGKILL qemu, idk what to do...");
    }
    q->pid = 0;

    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        un_log_errno(LOG_ERR, "failed to unblock SIGCHLD");
        return -errno;
    }

    return ret;
}

void qemu_destroy(qemu_t *q) {
    close(q->sock);
    free(q);
}

int qemu_initialize(qemu_t *q) {
    un_log(LOG_DEBUG, "initializing qemu");
    char *chardev;
    char *mon;
    int sock[2];
    int ret = 0;



    if(!q->preconfig) {
        return -1;
    }

    if(!xsockpair(sock)) {
        return -1;
    }

    q->sock = sock[0];

    mon = xaprintf(QMP_MON, "qmp0");
    chardev = xaprintf(QMP_CHARDEV_FD, "qmp0", sock[1]);

    un_log(LOG_DEBUG, "forking...");
    q->pid = fork();
    if(q->pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        dup2(devnull, STDIN_FILENO);
        close(devnull); /* close /dev/null */
        close(sock[0]); /* close parent side of sockpair */
        char *argv[] = { "qemu-system-x86_64", "--preconfig", "-mon", mon, "-chardev", chardev, NULL };
        un_log(LOG_DEBUG, "executing %s with following args: ", argv[0]);
        if(un_log_enabled(LOG_DEBUG)) {
            for(int i = 0; argv[i] != NULL; i++) {
                un_log(LOG_DEBUG, "\t%s", argv[i]);
            }
        }

        execvp(argv[0], argv);

        un_log_errno(LOG_ERR, "failed to exec qemu");
        exit(1);
    } else if(q->pid > 0) {
        close(sock[1]); /* close child side of sockpair */
        ret = 0;
    } else {
        un_log_errno(LOG_ERR, "failed to fork");
        ret = -1;
    }

    free(mon);
    free(chardev);
    return ret;
}
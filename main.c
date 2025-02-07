#include "log.h"
#include "net.h"
#include "qemu.h"
#include "util.h"
#include "client.h"

#include "jansson.h"

#include <sys/socket.h>
#include <arpa/inet.h>

#include <poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

size_t net_readcb(void *buffer, size_t buflen, void *data) {
    int fd = (int)(intptr_t)(data);

    un_log(LOG_DEBUG, "reading %d bytes from fd %d", buflen, fd);

    int64_t ret = net_read(fd, buffer, buflen);
    if(ret < 0) {
        return (size_t)-1;
    } else {
        un_log(LOG_DEBUG, "read %ld bytes", ret);
        return ret;
    }
}

static sig_atomic_t child = 0;
static sig_atomic_t term = 0;

static void sighandleterm(int sig) {
    term = 1;
}

static void sighandlechild(int sig) {
    child = 1;
}

int main(int argc, char **argv) {
    un_log(LOG_INFO, "starting up");

    int sv = sv_open("unix:///tmp/vtop.sock");
    if(sv < 0) {
        un_log_err(LOG_ERR, -sv, "err opening socket");
    }

    xnonblock(sv);

    if(listen(sv, 5) < 0) {
        un_log_errno(LOG_ERR, "listen");
        return -1;
    }

    struct pollfd *pfds = calloc(6, sizeof(struct pollfd));
    pfds[0] = (struct pollfd){.fd = sv, .events = POLLIN};

    client_t **clients = calloc(6, sizeof(client_t *));


    int ret = 0;
    int npfds = 1;
    int nclients = 0;

    for(;;) {
        ret = poll(pfds, npfds, -1);
        if(ret < 0) {
            if(errno != EINTR) {
                un_log_errno(LOG_ERR, "ppoll()");
                break;
            }
        }
        for(int i = 0; i < npfds; i++) {
            if(pfds[i].fd == sv && pfds[i].revents & POLLIN) {
                int client = accept(sv, NULL, NULL);
                if(client < 0) {
                    un_log_errno(LOG_ERR, "accept");
                    return 1;
                }
                clients[nclients++] = client_new(client);
                pfds[npfds++] = (struct pollfd){.fd = client, .events = POLLOUT};
            }
            for(int x = 0; x < nclients; x++) {
                if(clients[x] != NULL && pfds[i].fd == clients[x]->socket && (pfds[i].revents & POLLIN || pfds[i].revents & POLLOUT)) {
                    int status = client_handshake(clients[x]);
                    if(status == CLIENT_OK) {
                        un_log(LOG_INFO, "client handshake");
                    } else if(status == CLIENT_READ) {
                        pfds[i].events = POLLIN;
                    } else if(status == CLIENT_WRITE) {
                        pfds[i].events = POLLOUT;
                    } else {
                        un_log(LOG_ERR, "client error");
                        close(clients[x]->socket);
                        free(clients[x]);
                        clients[x] = NULL;
                    }
                }
            }            
        }

    }


    return 0;


    // char linebuf[4096];
    // int ret = 0;
    // struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
    // struct pollfd pfds[2];
    // sigset_t sigmask;
    // struct sigaction sa;

    // sigemptyset(&sigmask);
    // sigaddset(&sigmask, SIGCHLD);
    // sigaddset(&sigmask, SIGTERM);

    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;

    // sa.sa_handler = sighandleterm;
    // if(sigaction(SIGTERM, &sa, NULL) == -1) {
    //     un_log_errno(LOG_ERR, "signal: set TERM");
    //     return -1;
    // }
    // sa.sa_handler = sighandlechild;
    // if(sigaction(SIGCHLD, &sa, NULL) == -1) {
    //     un_log_errno(LOG_ERR, "signal: set CHILD");
    //     return -1;
    // }


    // for(;;) {
    //     ret = ppoll(pfds, 2, &timeout, &sigmask);
    //     if(ret < 0) {
    //         if(errno != EINTR) {
    //             un_log_errno(LOG_ERR, "ppoll()");
    //             break;
    //         }
    //     }

    //     for(int i = 0; i < ret; i++) {
    //         if(pfds[i].fd == q->stdout->fd) {
    //             char *p = pipebuf_readline(q->stdout, linebuf, sizeof(linebuf));
    //             if(!p && q->stdout->status != EAGAIN) {
    //                 un_log(LOG_ERR, "stdout err: %s", strerror(q->stdout->status));                    
    //             } else if(p) {
    //                 un_log(LOG_DEBUG, "qemu: %s", p);
    //             }
    //         } else if(pfds[i].fd == q->sock) {

    //         }

    //         if(term) {

    //         }
    //         else if(child) {
    //             /* if we have recieved a sigchild and we are not exiting child */
    //             if(!q->sigterm) {
    //                 un_log(LOG_ERR, "child unexpectedly exited");
    //             }
    //             ret = waitpid(q->pid, NULL, WNOHANG);
    //             if(ret < 0) {
    //                 un_log_errno(LOG_ERR, "waitpid: qemu");
    //             } else if(ret > 0) {
    //                 close(q->sock);
    //                 q->sock = -1;
    //                 pipebuf_destroy(q->stdout, 1);
    //                 q->stdout = NULL;
    //                 return -1;
    //             }
    //         }
    //     }
    // }

    return 0;
}


/*
{ "execute": "device_add", "arguments": { "core-id": 1, "driver":
*/
#include "server.h"

#include "util.h"
#include "log.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

server_t *server_new() {
    server_t *sv = calloc(1, sizeof(*sv));
    if(!sv) {
        return NULL;
    }
    sv->listenfd = -1;
    sv->clients = calloc(8, sizeof(client_t));
    sv->nclients = 0;
    sv->maxclients = 8;
}

int server_bind(server_t *sv, const char *addr) {
    int fd = sv_open(addr);
    if(fd < 0) {
        return -1;
    }
    sv->listenfd = fd;
    if(!xnonblock(sv->listenfd)) {
        close(sv->listenfd);
        sv->listenfd = -1;
        return -1;
    }

    if(listen(sv->listenfd, 8) < 0) {
        close(sv->listenfd);
        sv->listenfd = -1;
        un_log_errno(LOG_ERR, "listen error");
        return -1;
    }

    return 0;
}

int server_accept(server_t *sv) {
    int rc;

    for(;;) {
        rc = accept(sv->listenfd, NULL, NULL);
        if(rc < 0) {
            if(errno == EINTR) {
                continue;
            }
            return -errno;
        }
        break;
    }
    client_t *c = client_new(rc);

}
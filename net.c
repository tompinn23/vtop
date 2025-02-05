#include "net.h"

#include "log.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <arpa/inet.h>

int net_connect(const char *addr, int port) {
    char *portstr, *ip;
    char *endptr;
    long pval;
    int sock = -1;
    int ret = 0;

    portstr = strrchr(addr, ':');
    if(portstr == NULL && port > 0) {
        ip = strdup(addr);
    } else {
        ip = strndup(addr, (portstr - addr));
    }
    if(!ip) {
        return -ENOMEM;
    }

    if(port <= 0) {
        if(portstr == NULL) {
            ret = -EINVAL;
            goto err;
        }

        errno = 0;
        pval = strtol(portstr + 1, &endptr, 10);
        if(errno == ERANGE && (pval == LONG_MAX || pval == LONG_MIN)) {
            un_log_errno(LOG_ERR, "failed to parse port from '%s'", portstr);
            ret = -EINVAL;
            goto err;
        }
        if(endptr == portstr) {
            un_log(LOG_ERR, "no port number found in '%s'", portstr);
            ret = -EINVAL;
            goto err;
        } else if(*endptr != '\0') {
            un_log(LOG_ERR, "extra garbage found after port '%s' '%s'", portstr + 1, endptr);
            ret = -EINVAL;
            goto err;
        }
        
        if(pval < 0 || pval > 65535) {
            un_log(LOG_ERR, "port number '%ld' does not fall within port limits 0-65535", pval);
            ret = -ERANGE;
            goto err;
        }
        port = pval;
    }

    un_log(LOG_DEBUG, "connecting to '%s' at '%d'", ip, port);
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        un_log_errno(LOG_ERR, "failed to create socket");
        ret = -errno;
        goto err;
    }

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(ip);
    srv_addr.sin_port = htons(port);
    
    if(connect(sock, (struct sockaddr *)&srv_addr, (socklen_t)sizeof(srv_addr)) < 0) {
        un_log_errno(LOG_ERR, "failed to connect socket to %s %d", ip, port);
        ret = -errno;
        goto err;
    }

    ret = sock;
    sock = -1;
err:
    free(ip);
    if(sock != -1) {
        close(sock);
    }
    return ret;
}

int64_t net_read(int sock, char *buffer, size_t buflen) {
    ssize_t rc = 0;
    for(;;) {
        if((rc = read(sock, buffer, buflen)) < 0) {
            if(errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -errno;
        } else {
            return rc;
        }
    }
    return -1;
}
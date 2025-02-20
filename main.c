#include "log.h"
#include "util.h"

#include "vtop.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <limits.h>

#include <stdio.h>

int tcp_parse(const char *addr, struct sockaddr_storage *ss, socklen_t *len) {
    char *portstr, *ip;
    char *endptr, *ipend;
    long pval;
    int sock = -1;
    int ret = 0;

    sa_family_t family;

    ip = NULL;

    if(addr[0] == '[') {
        ipend = strrchr(addr, ']');
        ip = strndup(addr + 1, (ipend - addr) - 1);
        portstr = strrchr(ipend, ':');
        family = AF_INET6;
    } else {
        ipend = strrchr(addr, ':');
        ip = strndup(addr, (ipend - addr));
        portstr = strrchr(addr, ':');
        family = AF_INET;
    }
    
    if(ip == NULL) {
        ret = -ENOMEM;
        goto err;
    }

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
    
    if(family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)ss;
        if(inet_pton(AF_INET, ip, &sin->sin_addr) != 1) {
            un_log_errno(LOG_ERR, "failed to parse ipv4 '%s'", ip);
            ret = -EINVAL;
            goto err;
        }
        sin->sin_port = htons(pval);
        sin->sin_family = AF_INET;
        *len = sizeof(struct sockaddr_in);
    } else {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;
        if(inet_pton(AF_INET6, ip, &sin6->sin6_addr) != 1) {
            un_log_errno(LOG_ERR, "failed to parse ipv6 '%s'", ip);
            ret = -EINVAL;
            goto err;
        }
        sin6->sin6_port = htons(pval);
        sin6->sin6_family = AF_INET6;
        *len = sizeof(struct sockaddr_in6);
    }

err:
    free(ip);
    return ret;
}

int sv_open(const char *addr, int maxlisten) {
    struct sockaddr_storage ss;
    socklen_t len;
    int sock;
    if(strncmp(addr, "tcp://", 6) == 0) {
        int ret = tcp_parse(addr + 6, &ss, &len);
        if(ret < 0) {
            return ret;
        }
    } else if(strncmp(addr, "unix://", 7) == 0) {
        struct sockaddr_un *sun = (struct sockaddr_un *)&ss;
        sun->sun_family = AF_UNIX;
        strncpy(sun->sun_path, addr + 7, sizeof(sun->sun_path) - 1);
        sun->sun_path[sizeof(sun->sun_path)] = '\0';
        len = sizeof(struct sockaddr_un);
        unlink(sun->sun_path);
    } else {
        un_log(LOG_ERR, "unknown address scheme '%s'", addr);
        return -EINVAL;
    }
    un_log(LOG_DEBUG, "opening socket... '%s'", addr);
    sock = socket(ss.ss_family, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));

    if(bind(sock, (struct sockaddr *)&ss, len) < 0) {
        un_log_errno(LOG_ERR, "failed to bind socket");
        close(sock);
        return -errno;
    }

    if(listen(sock, maxlisten) < 0) {
        un_log_errno(LOG_ERR, "failed to listen to socket");
        close(sock);
        return -errno;
    }

    return sock;
}


int main(int argc, char **argv) {

    int fd = sv_open("tcp://127.0.0.1:8080", 1);

    run_loop(fd);
}
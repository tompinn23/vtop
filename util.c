#include "util.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "log.h"


char *xaprintf(const char *fmt, ...) {
    char *ret;
    va_list va;
    va_start(va, fmt);
    ret = xvaprintf(fmt, va);
    va_end(va);
    return ret;
}

char *xvaprintf(const char *fmt, va_list va) {
    va_list ap;
    va_copy(ap, va);
    size_t buf = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *buffer = malloc(sizeof(char) * (buf + 1));
    if(!buffer) {
        return NULL;
    }
    vsnprintf(buffer, buf + 1, fmt, va);
    buffer[buf] = '\0';
    return buffer;
}

ssize_t xread(int fd, char *buf, size_t len) {
    ssize_t ret = 0;
    for(;;) {
        ret = read(fd, buf, len);
        if(ret < 0 && errno == EINTR) {
            continue;
        }
        return ret;
    }
}

ssize_t xwrite(int fd, const void *buf, size_t len) {
    ssize_t ret = 0;
    for(;;) {
        ret = write(fd, buf, len);
        if(ret < 0 && errno == EINTR) {
            continue;
        }
        return ret;
    }
}

void timespec_sub(struct timespec *r, const struct timespec *a,
		const struct timespec *b) {
	r->tv_sec = a->tv_sec - b->tv_sec;
	r->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (r->tv_nsec < 0) {
		r->tv_sec--;
		r->tv_nsec += 1000000000;
	}
}

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

int sv_open(const char *addr) {
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

    if(bind(sock, (struct sockaddr *)&ss, len) < 0) {
        un_log_errno(LOG_ERR, "failed to bind socket");
        close(sock);
        return -errno;
    }

    return sock;
}

void xclosepair(int* fds) {
    close(fds[0]);
    close(fds[1]);
}

int xnonblock(int fd) {
    int flags;

    // Get the current flags for the socket
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        return 0;
    }

    // Set the socket to non-blocking mode
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return 0;
    }

    return 1;
}

fdbuf_t *fdbuf_new(int fd, int nlunix) {
    fdbuf_t *buf = calloc(1, sizeof(fdbuf_t));
    if(!buf) {
        return NULL;
    }

    buf->nlunix = nlunix;

    buf->fd = fd;
    buf->rbuffsize = LINE_INITIAL;
    buf->rbuff = malloc(LINE_INITIAL);
    if(!buf->rbuff) {
        free(buf);
        return NULL;
    }

    buf->rpos = 0;
    buf->wpos = 0;

    return buf;
}

void fdbuf_destroy(fdbuf_t *buf, int closefd) {
    if(closefd) {
        close(buf->fd);
    }
    free(buf->rbuff);
    free(buf);
}

int fdbuf_readline(fdbuf_t *buf, char *out, size_t buflen) {
    ssize_t bytes_read = 0;
    for(;;) {
        if(buf->rpos > 0) {
            char *nl = memchr(buf->rbuff, buf->nlunix ? '\n' : '\r', buf->rpos);
            if(nl && (buf->nlunix || (nl + 1 < buf->rbuff + buf->rpos && *(nl + 1) == '\n'))) {
                size_t len = nl - buf->rbuff;
                if(len + 1 > buflen) {
                    return FDBUF_ENOMEM;
                }
                memcpy(out, buf->rbuff, len);
                out[len] = '\0';
                memmove(buf->rbuff, nl + 1, buf->rpos - len - 1);
                buf->rpos -= len + 1;
                return len;
            }
        }
        if(buf->rpos == buf->rbuffsize) {
            if (buf->rbuffsize > SIZE_MAX / 2) {
                return FDBUF_ERROR;
            }
            buf->rbuffsize *= 2;
            buf->rbuff = realloc(buf->rbuff, buf->rbuffsize);
            if(!buf->rbuff) {
                return -1;
            }
        }
        bytes_read = xread(buf->fd, buf->rbuff + buf->rpos, buf->rbuffsize - buf->rpos);
        if(bytes_read < 0) {
            return errno == EAGAIN ? FDBUF_EAGAIN : FDBUF_ERROR;
        } else if(bytes_read == 0) {
            return FDBUF_ERROR;
        }
        buf->rpos += bytes_read;
    }

    return FDBUF_ERROR;
}

int fdbuf_write(fdbuf_t *buf, const char *data, size_t len) {
    ssize_t bytes_written = 0;

    if(data != NULL) {
        buf->wbuff = data;
        buf->wbuffsize = len;
        buf->wpos = 0;
    }
    bytes_written = xwrite(buf->fd, buf->wbuff + buf->wpos, buf->wbuffsize);
    if(bytes_written <= 0) {
        return errno == EAGAIN ? FDBUF_EAGAIN : FDBUF_ERROR;
    }
    buf->wpos += bytes_written;

    if(buf->wbuffsize == buf->wpos) {
        buf->wbuff = NULL;
        buf->wbuffsize = 0;
        buf->wpos = 0;
        return FDBUF_OK;
    }
    un_log(LOG_DEBUG, "reached unreachable end?");
    return FDBUF_ERROR;
}
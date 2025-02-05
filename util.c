#include "util.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


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

    return 0;
}

pipebuf_t *pipebuf_new(int fd) {
    pipebuf_t *buf = calloc(1, sizeof(*buf));
    buf->fd = fd;
    return buf;
}

void pipebuf_destroy(pipebuf_t *buf, int closefd) {
    if(closefd) {
        close(buf->fd);
    }
    free(buf);
}


char *pipebuf_readline(pipebuf_t *buf, char *out, size_t buflen) {
    size_t len;
    char *s;
    unsigned char *p, *t;

    if(buflen <= 0) {
        return NULL;
    }

    s = out;
    buflen--;
    while(buflen != 0) {
        if((len = buf->avail) <= 0) {
            ssize_t avail = xread(buf->fd, buf->buff, LINE_MAX);
            if(avail < 0) {
                buf->status = errno;
                return NULL;
            }
            buf->pos = 0;
            buf->avail = avail;
            len = buf->avail;
        }
        p = buf->buff + buf->pos;
        if(len > buflen) {
            len = buflen;
        }
        t = memchr(p, '\n', len);
        if(t != NULL) {
            len = ++t - p;
            buf->avail -= len;
            buf->pos += len;
            memcpy(s, p, len);
            s[len] = 0;
            return out;
        }
        buf->avail -= len;
        buf->pos += len;
    }
    *s = 0;
    return out;
}
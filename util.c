#include "util.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>

void *xmalloc(size_t n) {
    void *ptr = malloc(n);
    if(!ptr) {
        fputs("[vtop] malloc returned null\n",stderr);
        abort();
    }
    return ptr;
}
void *xcalloc(size_t n, size_t s) {
    void *ptr = calloc(n, s);
    if(!ptr) {
        fputs("[vtop] calloc returned null\n",stderr);
        abort();
    }
    return ptr;
}
void *xrealloc(void *o, size_t n) {
    void *ptr = realloc(o, n);
    if(!ptr) {
        fputs("[vtop] realloc returned null\n",stderr);
        abort();
    }
    return ptr;
}

struct iobuf *iobuf_create(size_t buffsize, void *data, 
    int64_t (*write)(void *data, char *buff, size_t len),
    int64_t (*read)(void *data, char *buff, size_t len)) {
    
    struct iobuf *buf = xcalloc(1, sizeof(*buf));
    buf->rbuffer = xmalloc(buffsize);
    buf->wbuffer = xmalloc(buffsize);
    buf->wpos = buf->rpos = 0;
    buf->wlen = buf->rlen = buffsize;
    
    buf->data = data;
    buf->write = write;
    buf->read = read;
    
    return buf;
}

int64_t fd_read(void *data, char *buff, size_t len) {
    int fd = (int)((intptr_t)data);

    ssize_t ret = 0;
    
    while((ret = read(fd, buff, len)) == -1 && errno == EINTR);

    if(ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return IOBUF_AGAIN;
    } else if(ret == -1) {
        return IOBUF_SYSERR;
    }
    return ret;
}

int64_t fd_write(void *data, char *buff, size_t len) {
    int fd = (int)((intptr_t)data);

    ssize_t ret = 0;
    while((ret = write(fd, buff, len)) == -1 && errno == EINTR);

    if(ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return IOBUF_AGAIN;
    } else if(ret == -1) {
        return IOBUF_SYSERR;
    }
    return ret;
}

struct iobuf *iobuf_createfd(size_t buffsize, int fd) {
    return iobuf_create(buffsize, (void*)((intptr_t)(fd)), fd_write, fd_read);
}

int64_t iobuf_readline(struct iobuf *buf, int crlf, char *buff, size_t max) {
    int skip = crlf ? 2 : 1;
    for(;;) {
        if(buf->rpos > 0) {
            char *nl = memchr(buf->rbuffer, crlf ? '\r' : '\n', buf->rpos);
            size_t len = (nl - buf->rbuffer);
            if(nl && (!crlf || len < buf->rpos - 1 && nl[1] == '\n')) {
                if(len > max) {
                    return IOBUF_NOSPACE;
                }
                memcpy(buff, buf->rbuffer, len);
                /* shift data forward but skip */
                memmove(buf->rbuffer, buf->rbuffer + len + skip, buf->rpos - len - skip);
                buf->rpos -= (len + skip);
                return len;
            }
        }
        if(buf->rpos == buf->rlen) {
            return IOBUF_FULL; /* buffer full */
        }
        int64_t ret = buf->read(buf->data, buf->rbuffer + buf->rpos, buf->rlen - buf->rpos);
        if(ret < 0) {
            return ret;
        }
        buf->rpos += ret;
    }
}

int64_t iobuf_read(struct iobuf *buf, char *buff, size_t len) {
    for(;;) {
        if(len >= buf->rlen) {
            return IOBUF_NOSPACE;
        }
        if(buf->rpos >= len) {
            memcpy(buff, buf->rbuffer, len);
            /* shift data forward */
            memmove(buf->rbuffer, buf->rbuffer + len, buf->rpos - len);
            buf->rpos -= len;
            return len;
        }
        int64_t ret = buf->read(buf->data, buf->rbuffer + buf->rpos, buf->rlen - buf->rpos);
        if(ret < 0) {
            return ret;
        }
        buf->rpos += ret;
    }
}

int64_t iobuf_write(struct iobuf *buf, char *buff, size_t len) {
    int64_t ret = 0;
    ret = iobuf_flush(buf);
    if(ret < 0 && (ret != IOBUF_AGAIN)) {
        return ret; /* some error that isn't pollagain */
    }
    if(len < buf->wlen - buf->wpos) {
        memcpy(buf->wbuffer + buf->wlen, buff, len);
        buf->wpos += buf->wlen;
    } else {
        return IOBUF_NOSPACE;
    }
    return iobuf_flush(buf);
}

int64_t iobuf_flush(struct iobuf *buf) {
    int64_t ret = 0;
    if(buf->wpos == 0) {
        return 0; // nothing to write out to wire
    }
    int64_t total_written = 0;
    for(;;) {
        if(buf->wpos > 0) {
            /* first try to push existing buffered data */
            ret = buf->write(buf->data, buf->wbuffer, buf->wpos);
            if(ret > 0) {
                /* if we wrote something move the data forward */
                memmove(buf->wbuffer, buf->wbuffer + buf->wpos, ret);
                buf->wpos -= ret;
                total_written += ret;
            }
            if(ret < 0) {
                return ret;
            }
        } else {
            return total_written;
        }
    }
}


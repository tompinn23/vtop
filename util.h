#pragma once

#include <stddef.h>
#include <stdint.h>

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t s);
void *xrealloc(void *o, size_t n);

typedef struct iobuf {
    char *rbuffer;
    char *wbuffer;
    size_t rpos, wpos, rlen, wlen;

    void *data;
    int64_t (*write)(void *data, char *buff, size_t len);
    int64_t (*read)(void *data, char *buff, size_t len);
} iobuf_t;

struct iobuf *iobuf_create(size_t buffsize, void *data, 
    int64_t (*write)(void *data, char *buff, size_t len),
    int64_t (*read)(void *data, char *buff, size_t len));

struct iobuf *iobuf_createfd(size_t buffsize, int fd);
int64_t iobuf_readline(struct iobuf *buf, int crlf, char *buff, size_t max);
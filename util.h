#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t s);
void *xrealloc(void *o, size_t n);

#define IOBUF_ERROR (-1)
#define IOBUF_AGAIN (-2)
#define IOBUF_SYSERR (-3)
#define IOBUF_FULL (-4)
#define IOBUF_NOSPACE (-5)

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

int64_t iobuf_write(struct iobuf *buf, char *buff, size_t len);
int64_t iobuf_flush(struct iobuf *buf);

int64_t iobuf_writef(struct iobuf *buf, const char *fmt, ...);
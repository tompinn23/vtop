#pragma once

#include <stdarg.h>
#include <stdint.h>

#define LINE_MAX 4096

typedef struct pipebuf {
    int fd;
    int status;
    char buff[LINE_MAX];
    size_t pos;
    int64_t avail;
} pipebuf_t;

char *xaprintf(const char *fmt, ...);
char *xvaprintf(const char *fmt, va_list va);

int xnonblock(int fd);
void xclosepair(int fds);

pipebuf_t *pipebuf_new(int fd);
void pipebuf_destroy(pipebuf_t *buf, int closefd);
char *pipebuf_readline(pipebuf_t *buf, char *out, size_t buflen);
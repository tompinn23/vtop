#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>

#define LINE_INITIAL 2048

#define FDBUF_EAGAIN -10
#define FDBUF_ENOMEM -5
#define FDBUF_ERROR -1
#define FDBUF_OK 0

typedef struct fdbuf {
    int fd;
    int nlunix;
    char *rbuff, *wbuff;
    size_t rpos, wpos;
    size_t rbuffsize, wbuffsize;
} fdbuf_t;

char *xaprintf(const char *fmt, ...);
char *xvaprintf(const char *fmt, va_list va);

int sv_open(const char *addr);

ssize_t xread(int fd, char *buf, size_t len);
ssize_t xwrite(int fd, const void *buf, size_t len);


int xnonblock(int fd);
void xclosepair(int* fds);


void timespec_sub(struct timespec *r, const struct timespec *a,
		const struct timespec *b);

/**
 * Allocate a new file descriptor buffer.
 * 
 * @param fd the file descriptor to read/write from
 * @param nlunix if the newline character is unix style
 * 
 * @return a new file descriptor buffer
 */
fdbuf_t *fdbuf_new(int fd, int nlunix);
void fdbuf_destroy(fdbuf_t *buf, int closefd);
int fdbuf_readline(fdbuf_t *buf, char *out, size_t buflen);
int fdbuf_write(fdbuf_t *buf, const char *data, size_t len);
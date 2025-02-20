#include "proto.h"

#include <stdio.h>

int64_t read_message(struct iobuf *buf, struct message **msg) {
    char linebuf[4096];
    int64_t ret = 0;
    ret = iobuf_readline(buf, 1, linebuf, sizeof(linebuf) - 1);
    if(ret < 0) {
        return ret;
    }
    linebuf[ret] = '\0';

}

int64_t write_message(struct iobuf *buf, const char *fmt, ...) {
    size_t amount = 0;
    int64_t ret = 0;
    va_list va, ap;

    va_start(va, fmt);
    va_copy(ap, va);
    amount = vsnprintf(NULL, 0, fmt, va);
    ret = iobuf_flush(buf);
    if(ret < 0 && (ret != IOBUF_AGAIN)) {
        return ret; /* some error that isn't pollagain */
    }
    if(amount + 2 < buf->wlen - buf->wpos) {
        amount = vsnprintf(buf->wbuffer + buf->wpos, buf->wlen - buf->wpos, fmt, ap);
        (buf->wbuffer + buf->wpos)[amount] = '\r';
        (buf->wbuffer + buf->wpos)[amount + 1] = '\n';
        buf->wpos += (amount + 2);
        return iobuf_flush(buf);
    }
    return IOBUF_NOSPACE;
}
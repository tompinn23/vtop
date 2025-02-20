#pragma once

#include "util.h"

struct message {
    const char *message;
    const char *arguments[32];
};

int64_t read_message(iobuf_t *buf, struct message **msg);
int64_t write_message(iobuf_t *buf, const char *fmt, ...);
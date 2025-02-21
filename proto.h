#pragma once

#include "util.h"

#define MSG_OK (0)
#define MSG_UNKNOWN_CMD (-1)
#define MSG_INCORRECT_ARGS (-2)

struct message {
    int type;
};

struct helo_message {
    struct message base;
    char *svname;
    char *clientname;
};

enum token_type {
    /* generic arguments */
    TOKEN_IDENTIIFER, /* unquoted string no whitespace */
    TOKEN_STRING, /* quoted string */
    TOKEN_NUMBER, /* number */

    /* commands */
    TOKEN_CMD_START,
    TOKEN_HELO,
    TOKEN_CAP,
    TOKEN_CMD_END,

    TOKEN_ERROR, TOKEN_END
};

struct scanner {
    const char *buf;
    size_t len;
    size_t pos;

    const char *start, *current;
};

struct token {
    enum token_type type;
    const char *start;
    size_t len;
};

void scanner_init(struct scanner *s, const char *buf, size_t len);
struct token scan_token(struct scanner *s);


int64_t read_message(char *buf, size_t len, struct message **msg);
int64_t write_message(iobuf_t *buf, const char *fmt, ...);
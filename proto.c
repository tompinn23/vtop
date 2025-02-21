#include "proto.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h>

static int is_end(struct scanner *s) {
    return s->pos >= s->len;
}

static struct token make_token(struct scanner *s, enum token_type type) {
    struct token t;
    t.type = type;
    t.start = s->start;
    t.len = s->current - s->start;
    return t;
}

static struct token error_token(struct scanner *s, const char *msg) {
    struct token t;
    t.type = TOKEN_ERROR;
    t.start = msg;
    t.len = strlen(msg);
    return t;
}

static char advance(struct scanner *s) {
    if(s->pos < s->len) {
        s->pos++;
        s->current++;
        return s->current[-1];
    }
    return '\0';
} 

static char peek(struct scanner *s) {
    return *s->current;
}

static void skip_ws(struct scanner *s) {
    for (;;) {
      char c = peek(s);
      switch (c) {
        case ' ':
        case '\t':
          advance(s);
          break;
        default:
          return;
      }
    }
}

static struct token string(struct scanner *s) {
    while(peek(s) != '"' && !is_end(s)) {
        advance(s);
    }
    if(is_end(s)) {
        return error_token(s, "unterminated string");
    }
    advance(s);
    return make_token(s, TOKEN_STRING);
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_space(char c) {
    return c == ' ' || c == '\t';
}

static struct token number(struct scanner *s) {
    while(is_digit(peek(s))) {
        advance(s);
    }
    return make_token(s, TOKEN_NUMBER);
}

static enum token_type check_keyword(struct scanner *s, int start, int length, const char *rest, enum token_type type) {
    if(s->current - s->start == start + length && memcmp(s->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIIFER;
}

static enum token_type identifier_type(struct scanner *s) {
    switch(s->start[0]) {
        case 'H': return check_keyword(s, 1, 3, "ELO", TOKEN_HELO);
        case 'C': return check_keyword(s, 1, 2, "AP", TOKEN_CAP);
    }
    return TOKEN_IDENTIIFER;
}
static struct token identifier(struct scanner *s) {
    while(!is_space(peek(s)) && !is_end(s)) {
        advance(s);
    }
    return make_token(s, identifier_type(s));
}


void scanner_init(struct scanner *s, const char *buf, size_t len) {
    s->buf = buf;
    s->len = len;
    s->pos = 0;
    s->start = s->current = s->buf;
}

struct token scan_token(struct scanner *s) {
    skip_ws(s);
    s->start = s->current;

    if(is_end(s)) return make_token(s, TOKEN_END);

    char c = advance(s);

    if(is_digit(c)) return number(s);
    if(c == '"') return string(s);
    else return identifier(s);
    return error_token(s, "unexpected character");
}

int64_t read_message(char *buf, size_t len, struct message **msg) {
    struct scanner s;
    scanner_init(&s, buf, len);
    struct token t = scan_token(&s);
    if(!(t.type > TOKEN_CMD_START && t.type < TOKEN_CMD_END)) {
        return MSG_UNKNOWN_CMD;
    }
    switch(t.type) {
        case TOKEN_HELO: {
            struct helo_message *hm = xcalloc(1, sizeof(*hm));
            hm->base.type = t.type;
            struct token t = scan_token(&s);
            if(t.type != TOKEN_STRING) {
                free(hm);
                return MSG_INCORRECT_ARGS;
            }
            hm->svname = strndup(t.start, t.len);
            t = scan_token(&s);
            if(t.type != TOKEN_STRING) {
                free(hm->svname);
                free(hm);
                return MSG_INCORRECT_ARGS;
            }
            hm->clientname = strndup(t.start, t.len);
            *msg = (struct message *)hm;
            return MSG_OK;
        }
        default:
            return MSG_UNKNOWN_CMD;
    }
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
#include "client.h"

#include "log.h"
#include "util.h"

#include "proto.h"

#include <stdlib.h>

enum {
    ERROR,
    INITIAL,
    HELLO,
    MAIN,
    MAX_STATES,
};

typedef int (*action_fn)(struct client *, struct iobuf *);

int invalid(struct client *c, iobuf_t *iobuf) {
    un_log(LOG_DEBUG, "client ended up in invalid state: %d", c->state);
}

int send_initial(struct client *c, iobuf_t *iobuf) {
    return write_message(iobuf, "HELO <vtop.2>");
}
int send_error(struct client *c, iobuf_t *iobuf) {
    int64_t ret = write_message(iobuf, "%d-ERR: %s", c->errcode, c->errmsg);
    if(ret < 0) {
        return CLIENT_WRITE;
    }
    return 0;
}

int receive_hello(struct client *c, iobuf_t *iobuf) {
    struct message *m;
    char linebuf[4096];
    int64_t ret = 0;
    ret = iobuf_readline(iobuf, 1, linebuf, sizeof(linebuf));
    if(ret < 0) {
        if(ret == IOBUF_AGAIN) {
            return CLIENT_READ;
        } else {
            return -1;
        }
    }
    ret = read_message(linebuf, ret, &m);
    switch(ret) {
        case MSG_OK:
            struct helo_message *hm = (struct helo_message *)m;
            un_log(LOG_INFO, "received HELO from %s", hm->clientname);
            c->state = HELLO;
            free(hm);
            break;
        case MSG_INCORRECT_ARGS:
            c->errcode = 400;
            c->errmsg = "incorrect arguments";
            c->state = ERROR;
            break;
        case MSG_UNKNOWN_CMD:
            c->errcode = 404;
            c->errmsg = "unknown command";
            c->state = ERROR;
            break;
    }
    return 0;
}


struct client *client_new() {
    struct client *c = xcalloc(1, sizeof(*c));
    c->state = INITIAL;

    return c;
}

int client_handshake(struct client *c, struct iobuf *iobuf) {
    int64_t ret = 0;
    if(c->state == INITIAL) {
        ret = write_message(iobuf, "HELO <vtop.2>");
        if(ret < 0) {
            return ret;
        }
        c->state = HELLO;
        return CLIENT_READ;
    } else if(c->state == HELLO) {
        ret = receive_hello(c, iobuf);
        if(ret < 0) {
            return ret;
        } else if(ret == 1) {
            c->state = MAIN;
            return CLIENT_READ;
        }
        return 0;
    } else if(c->state == ERROR) {
       ret = send_error(c, iobuf);
       if(ret < 0) {
            return ret;
       }
       c->state = HELLO;
       return CLIENT_READ;
    }
    return -1;
}
#include "client.h"

#include <stdlib.h>
#include "log.h"


client_t *client_new(int socket) {
    client_t *c = calloc(1, sizeof(client_t));
    if(!c) {
        return NULL;
    }

    c->socket = socket;
    if(!xnonblock(socket)) {
        free(c);
        return NULL;
    }

    c->state = CLIENT_STATE_NEW;
    c->buf = fdbuf_new(socket, 0);
    if(!c->buf) {
        free(c);
        return NULL;
    }
    return c;
}

int client_handshake(client_t *c) {
    int ret;
    if(c->state == CLIENT_STATE_NEW) {
        char *msg = "200 vtop 1.0\r\n";
        ret = fdbuf_write(c->buf, c->inmsg ? NULL : msg, strlen(msg));
        if(ret == FDBUF_ERROR) {
            return -1;
        } else if(ret == FDBUF_EAGAIN) {
            c->inmsg = 1;
        } else if(ret == FDBUF_OK) {
            c->state = CLIENT_STATE_RECV_HELLO;
            c->inmsg = 0;
            return CLIENT_READ;
        }
        if(c->inmsg) {
            return CLIENT_WRITE;
        }
    } else if(c->state == CLIENT_STATE_ERROR) {
        ret = fdbuf_write(c->buf, c->inmsg ? NULL : c->errmsg, strlen(c->errmsg));
        if(ret == FDBUF_ERROR) {
            return -1;
        } else if(ret == FDBUF_EAGAIN) {
            c->inmsg = 1;
        } else if(ret == FDBUF_OK) {
            c->state = CLIENT_STATE_RECV_HELLO;
            c->inmsg = 0;
        }
        if(c->inmsg) {
            return CLIENT_WRITE;
        }
    } else if(c->state == CLIENT_STATE_RECV_HELLO) {
        char line[4096];
        ret = fdbuf_readline(c->buf, line, sizeof(line));
        if(ret == FDBUF_ERROR) {
            return -1;
        } else if(ret == FDBUF_EAGAIN) {
            return CLIENT_READ;
        } else if(ret == FDBUF_ENOMEM) {
            c->state = CLIENT_STATE_ERROR;
            c->errcode = 410;
            c->errmsg = "message too long";
            return CLIENT_WRITE;
        } else if(ret > 0) {
            un_log(LOG_DEBUG, "client: %s", line);
            c->state = CLIENT_STATE_SEND_HELLO;
        }
    } else if(c->state == CLIENT_STATE_SEND_HELLO) {
    }

    return CLIENT_OK;
}
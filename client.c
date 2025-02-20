#include "client.h"

#include "log.h"
#include "util.h"

#include "proto.h"

enum {
    ERROR,
    INITIAL,
    HELLO,
    MAX_STATES,
};

typedef int (*action_fn)(struct client *, struct iobuf *, va_list);

int invalid(struct client *c, iobuf_t *iobuf, va_list va) {
    un_log(LOG_DEBUG, "client ended up in invalid state: %d", c->state);
}

int send_initial(struct client *c, iobuf_t *iobuf, va_list va) {
    return write_message(iobuf, "HELO <vtop.2>");
}
int send_error(struct client *c, iobuf_t *iobuf, va_list va) {
    
}
int send_hello(struct client *c, iobuf_t *iobuf, va_list va) {

}
int receive_hello(struct client *c, iobuf_t *iobuf, va_list va) {
    
}

const action_fn states[MAX_STATES][2] = {
    [ERROR] = { invalid, send_error },
    [INITIAL] = { invalid, send_initial },
    [HELLO]  = { receive_hello, send_hello },
};

struct client *client_new() {
    struct client *c = xcalloc(1, sizeof(*c));
    c->state = INITIAL;

    return c;
}

int client_state(struct client *c, struct iobuf *iobuf, int event, ...) {
    va_list va;
    va_start(va, event);
    int ret = states[c->state][event](c, iobuf, va);
    va_end(va);
    return ret;
}


int client_process(struct client *c, struct iobuf *iobuf, int event) {
    int ret = 0;
    for(;;) {
        ret = client_state(c, iobuf, event)
        if(ret < 0) {
            return ret;
        }
    }
    return -1;
}


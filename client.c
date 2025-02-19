#include "client.h"

#include "log.h"
#include "util.h"

enum {
    ERROR,
    INITIAL,
    HELLO,
    MAX_STATES,
};

typedef int (*action_fn)(struct client *);

int invalid(struct client *c) {
    un_log(LOG_DEBUG, "client ended up in invalid state: %d", c->state);
}

int send_initial(struct client *c) {

}
int send_error(struct client *c) {

}
int send_hello(struct client *c) {

}
int receive_hello(struct client *c) {
    
}

const action_fn states[MAX_STATES][2] = {
    [ERROR] = { invalid, send_error },
    [INITIAL] = { invalid, send_initial },
    [HELLO]  = { receive_hello, send_hello },
};

struct client *client_new(int sock) {
    struct client *c = xcalloc(1, sizeof(*c));
    c->fd = sock;
    c->state = INITIAL;
}


int client_pump(struct client *c, int event) {
    int ret = 0;
    for(;;) {
        ret = states[c->state][event](c);
        if(ret < 0) {
            return ret;
        }
    }
    return -1;
}


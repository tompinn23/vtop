#include "log.h"
#include "net.h"
#include "qemu.h"
#include "util.h"

#include "jansson.h"

#include <sys/socket.h>
#include <arpa/inet.h>

size_t net_readcb(void *buffer, size_t buflen, void *data) {
    int fd = (int)(intptr_t)(data);

    un_log(LOG_DEBUG, "reading %d bytes from fd %d", buflen, fd);

    int64_t ret = net_read(fd, buffer, buflen);
    if(ret < 0) {
        return (size_t)-1;
    } else {
        un_log(LOG_DEBUG, "read %ld bytes", ret);
        return ret;
    }
}

int main(int argc, char **argv) {
    un_log(LOG_INFO, "starting up");

    qemu_t *q = qemu_new();
    qemu_initialize(q);


    char *d = xaprintf("Hello, %s", "world");
    puts(d);
    
    char buf[1024];
    net_read(q->sock, buf, sizeof(buf));

    sleep(5);

    qemu_terminate(q);

    return 0;
}


/*
{ "execute": "device_add", "arguments": { "core-id": 1, "driver":
*/
#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct qemu {
    int sock;
    pid_t pid;
    bool sigterm;


    bool preconfig;


} qemu_t;

typedef struct qemu_info {
    char **commands;
    size_t commands_len;

    char **cpu_models;
    size_t cpu_models_len;
} qemu_info_t;

#define QMP_CHARDEV_FD "socket,id=%s,fd=%d"
#define QMP_MON "chardev=%s,mode=control"

/**
 * Allocate the memory for a new QEMU instance.
 */
qemu_t *qemu_new(void);
/**
 * Initialize the QEMU instance.
 * 
 * INFO: Will start a new QEMU process
 */
int qemu_initialize(qemu_t *q);
int qemu_terminate(qemu_t *q);
void qemu_destroy(qemu_t *q);



qemu_info_t *qemu_info_new(void);
void qemu_info_free(qemu_info_t *qi);

/**
 * Query QEMU for information such as cpu models.
 * 
 * INFO: this will spawn a qemu process 
 */
int qemu_info_initialize(qemu_info_t *qi);


#pragma once

#include <stddef.h>
#include <stdint.h>

int net_connect(const char *addr, int port);

int64_t net_read(int sock, char *buffer, size_t buflen);
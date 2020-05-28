#pragma once

#include "net/net-connections.h"

void start_master(int *http_fd, int (*try_get_http_fd)(), int http_fd_port);

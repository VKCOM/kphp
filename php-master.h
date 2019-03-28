#pragma once

#include "net/net-connections.h"
void start_master(int *http_fd, int (*try_get_http_fd)(void), int http_fd_port);
struct connection *create_pipe_reader(int pipe_fd, conn_type_t *type, void *extra);


#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <sys/cdefs.h>

#include "net/net-connections.h"

enum pipe_type {
  pipe_for_read,
  pipe_for_write
};
struct connection *epoll_insert_pipe(enum pipe_type type, int pipe_fd, conn_type_t *conn_type, void *extra);


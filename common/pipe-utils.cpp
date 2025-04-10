// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/pipe-utils.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "net/net-connections.h"
#include "net/net-tcp-rpc-common.h"

#include "common/crc32c.h"
#include "common/server/limits.h"

struct connection* epoll_insert_pipe(enum pipe_type type, int pipe_fd, conn_type_t* conn_type, void* extra, int extra_flags) {
  if (check_conn_functions(conn_type) < 0) {
    return NULL;
  }
  if (pipe_fd >= MAX_CONNECTIONS || pipe_fd < 0) {
    return NULL;
  }

  event_t* ev;
  ev = epoll_fd_event(pipe_fd);
  struct connection* c = Connections + pipe_fd;
  memset(c, 0, sizeof(struct connection));

  c->fd = pipe_fd;
  c->ev = ev;
  c->generation = ++conn_generation;

  switch (type) {
  case pipe_for_read:
    c->flags = C_WANTRD | C_RAWMSG;
    break;
  case pipe_for_write:
    c->flags = C_WANTWR;
    break;
  }

  init_connection_buffers(c);
  c->timer.wakeup = conn_timer_wakeup_gateway;
  c->type = conn_type;
  c->extra = extra;
  c->basic_type = ct_pipe;
  active_connections++;
  c->first_query = c->last_query = (struct conn_query*)c;

  // TODO: it is safe, but maybe move to previous `switch`
  switch (type) {
  case pipe_for_read:
    c->status = conn_wait_answer;
    TCP_RPC_DATA(c)->custom_crc_partial = crc32c_partial;
    break;
  case pipe_for_write:
    c->status = conn_write_close;
    c->write_timer.wakeup = conn_write_timer_wakeup_gateway;
    break;
  }

  epoll_sethandler(pipe_fd, 0, server_read_write_gateway, c);
  epoll_insert(pipe_fd, (c->flags & C_WANTRD ? EVT_READ : 0) | (c->flags & C_WANTWR ? EVT_WRITE : 0) | extra_flags);
  return c;
}

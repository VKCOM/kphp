// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-events.h"
#include "server/external-net-drivers/connector.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/php-queries.h"
#include "server/external-net-drivers/request.h"

bool Connector::connected() const noexcept {
  return is_connected;
}

bool Connector::connect_async() noexcept {
  if (connected()) {
    return true;
  }
  is_connected = connect_async_impl();
  if (is_connected) {
    int fd = get_fd();
    epoll_insert(fd, EVT_SPEC | EVT_LEVEL);
    // store request-bound connection_id to data. We can't simply store `this` because it's pointer to script memory. But answer
    epoll_sethandler(fd, 0, NetDriversAdaptor::epoll_gateway, this);
  }
  return is_connected;
}

void Connector::update_state_ready_to_write() {
  fprintf(stderr, "@@@@@@@ update_state_ready_to_write\n");
  ready_to_write = true;
  ready_to_read = false;
  update_state_in_reactor();
}

void Connector::update_state_ready_to_read() {
  fprintf(stderr, "@@@@@@@ update_state_ready_to_read\n");
  ready_to_read = true;
  ready_to_write = false;
  update_state_in_reactor();
}

void Connector::update_state_idle() {
  fprintf(stderr, "@@@@@@@ update_state_idle\n");
  ready_to_write = false;
  ready_to_read = false;
  update_state_in_reactor();
}

void Connector::update_state_in_reactor() const noexcept {
  int action_flags = 0;
  if (ready_to_read) {
    action_flags |= EVT_READ;
  }
  if (ready_to_write) {
    action_flags |= EVT_WRITE;
  }
  epoll_insert(get_fd(), EVT_SPEC | EVT_LEVEL | action_flags);
}

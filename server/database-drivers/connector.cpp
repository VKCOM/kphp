// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/connector.h"

#include "net/net-events.h"
#include "runtime/critical_section.h"
#include "server/database-drivers/adaptor.h"
#include "server/php-queries.h"

namespace database_drivers {

bool Connector::connected() const noexcept {
  return is_connected;
}

AsyncOperationStatus Connector::connect_async_and_epoll_insert() noexcept {
  dl::CriticalSectionGuard guard;
  if (connected()) {
    return AsyncOperationStatus::COMPLETED;
  }
  auto status = connect_async();
  switch (status) {
    case AsyncOperationStatus::COMPLETED: {
      is_connected = true;
      int fd = get_fd();
      epoll_insert(fd, EVT_SPEC | EVT_LEVEL);
      epoll_sethandler(fd, 0, Adaptor::epoll_gateway, reinterpret_cast<void *>(static_cast<int64_t>(connector_id)));
      return AsyncOperationStatus::COMPLETED;
    }
    case AsyncOperationStatus::IN_PROGRESS:
    case AsyncOperationStatus::ERROR:
    default:
      return status;
  }
}

void Connector::update_state_ready_to_write() {
  ready_to_write = true;
  ready_to_read = false;
  update_state_in_reactor();
}

void Connector::update_state_ready_to_read() {
  ready_to_read = true;
  ready_to_write = false;
  update_state_in_reactor();
}

void Connector::update_state_idle() {
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

Connector::~Connector() noexcept {}

} // namespace database_drivers

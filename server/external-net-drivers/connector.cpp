// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-events.h"
#include "server/external-net-drivers/connector.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/external-net-drivers/request.h"

//void Connector::handle_write() noexcept {
//  for (auto it = pending_requests.cbegin(); it != pending_requests.cend(); ++it) {
//    Request *req = it.get_value();
//    bool completed = req->send_async();
//    php_assert(completed);
//  }
//  pending_requests.clear();
//}

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
    epoll_sethandler(fd, 0, NetDriversAdaptor::epoll_gateway, this);
  }
  return is_connected;
}


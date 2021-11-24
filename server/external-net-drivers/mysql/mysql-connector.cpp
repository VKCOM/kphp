// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>

#include "runtime/pdo/mysql/mysql_pdo.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "server/external-net-drivers/mysql/mysql-request.h"
#include "server/external-net-drivers/mysql/mysql-response.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/php-engine.h"

void MysqlConnector::close() noexcept {
  mysql_close(ctx);
}

int MysqlConnector::get_fd() const noexcept {
  if (!is_connected) {
    return -1;
  }
  return ctx->net.fd;
}

bool MysqlConnector::connect_async_impl() noexcept {
  if (is_connected) {
    return true;
  }
  net_async_status status =
    LIB_MYSQL_CALL(mysql_real_connect_nonblocking(ctx, host.c_str(), user.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0));
  return is_connected = (status == NET_ASYNC_COMPLETE);
}

void MysqlConnector::handle_read() noexcept {
  auto *response = new MysqlResponse{this};
  response->fetch();
  int event_status = vk::singleton<NetDriversAdaptor>::get().create_external_db_response_net_event(cur_request_id, response);
  cur_request_id = 0;
  on_net_event(event_status); // wakeup php worker to make it process new net event and continue corresponding resumable
}

void MysqlConnector::handle_write() noexcept {
  assert(connected());
  if (pending_request) {
    pending_request->send_async();
    pending_request = nullptr;
  }
}

void MysqlConnector::push_async_request(int request_id, Request *req) noexcept {
  if (cur_request_id) {
    assert(false); // no pipelining
  }
  pending_request = req;
  cur_request_id = request_id;
}

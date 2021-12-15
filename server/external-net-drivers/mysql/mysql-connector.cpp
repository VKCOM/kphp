// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>

#include "server/external-net-drivers/mysql/mysql.h"
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

  tvkprintf(mysql, 1, "MySQL try to connect to [%s:%d]: status = %d\n", host.c_str(), port, status);
  return is_connected = (status == NET_ASYNC_COMPLETE);
}

void MysqlConnector::push_async_request(Request *req) noexcept {
  assert(pending_request == nullptr && pending_response == nullptr && "Pipelining is not allowed");
  pending_request = req;

  tvkprintf(mysql, 1, "MySQL initiate async request send: request_id = %d\n", req->request_id);
  update_state_ready_to_write();
}

void MysqlConnector::handle_write() noexcept {
  assert(connected());
  assert(pending_request);

  bool completed = pending_request->send_async();
  if (!completed) {
    return;
  }

  pending_response = new MysqlResponse{this};
  pending_response->bound_request_id = pending_request->request_id;
  pending_request = nullptr;

  update_state_ready_to_read(); // TODO: maybe do it via EVA_CONTINUE | flags?
}

void MysqlConnector::handle_read() noexcept {
  assert(pending_response);

  bool completed = pending_response->fetch_async();
  if (!completed) {
    return;
  }

  int event_status = vk::singleton<NetDriversAdaptor>::get().create_external_db_response_net_event(pending_response);
  on_net_event(event_status); // wakeup php worker to make it process new net event and continue corresponding resumable

  pending_response = nullptr;
  update_state_idle();
}

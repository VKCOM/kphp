// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql-connector.h"

#include <mysql/mysql.h>

#include "server/database-drivers/mysql/mysql.h"
#include "server/database-drivers/mysql/mysql-request.h"
#include "server/database-drivers/mysql/mysql-response.h"
#include "server/database-drivers/adaptor.h"
#include "server/php-engine.h"

namespace database_drivers {

MysqlConnector::MysqlConnector(MYSQL *ctx, string host, string user, string password, string db_name, int port)
  : ctx(ctx)
  , host(std::move(host))
  , user(std::move(user))
  , password(std::move(password))
  , db_name(std::move(db_name))
  , port(port) {}

MysqlConnector::~MysqlConnector() noexcept {
  if (is_connected) {
    epoll_remove(get_fd());
    LIB_MYSQL_CALL(mysql_close(ctx));
    tvkprintf(mysql, 1, "MySQL disconnected from [%s:%d]: connector_id = %d\n", host.c_str(), port, connector_id);
  }
}

int MysqlConnector::get_fd() const noexcept {
  if (!is_connected) {
    return -1;
  }
  return ctx->net.fd;
}

AsyncOperationStatus MysqlConnector::connect_async() noexcept {
  if (is_connected) {
    return AsyncOperationStatus::COMPLETED;
  }
  net_async_status status =
    LIB_MYSQL_CALL(mysql_real_connect_nonblocking(ctx, host.c_str(), user.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0));

  tvkprintf(mysql, 1, "MySQL try to connect to [%s:%d]: connector_id = %d, status = %d\n", host.c_str(), port, connector_id, status);
  switch (status) {
    case NET_ASYNC_NOT_READY:
      return AsyncOperationStatus::IN_PROGRESS;
    case NET_ASYNC_COMPLETE:
      return AsyncOperationStatus::COMPLETED;
    case NET_ASYNC_ERROR:
    default:
      return AsyncOperationStatus::ERROR;
  }
}

void MysqlConnector::push_async_request(std::unique_ptr<Request> &&request) noexcept {
  dl::CriticalSectionGuard guard;

  assert(pending_request == nullptr && pending_response == nullptr && "Pipelining is not allowed");

  tvkprintf(mysql, 1, "MySQL initiate async request send: request_id = %d\n", request->request_id);

  pending_request = std::move(request);

  update_state_ready_to_write();
}

void MysqlConnector::handle_write() noexcept {
  assert(connected());
  assert(pending_request);

  AsyncOperationStatus status = pending_request->send_async();
  switch (status) {
    case AsyncOperationStatus::IN_PROGRESS:
      return;
    case AsyncOperationStatus::COMPLETED: {
      pending_response = std::make_unique<MysqlResponse>(connector_id, pending_request->request_id);
      pending_request = nullptr;
      update_state_ready_to_read(); // TODO: maybe do it via EVA_CONTINUE | flags?
      break;
    }
    case AsyncOperationStatus::ERROR: {
      auto response = std::make_unique<MysqlResponse>(connector_id, pending_request->request_id);
      response->is_error = true;
      pending_request = nullptr;
      vk::singleton<Adaptor>::get().finish_request_resumable(std::move(response));
      update_state_idle();
      break;
    }
  }
}

void MysqlConnector::handle_read() noexcept {
  assert(pending_response);

  auto &adaptor = vk::singleton<database_drivers::Adaptor>::get();
  AsyncOperationStatus status = pending_response->fetch_async();
  switch (status) {
    case AsyncOperationStatus::IN_PROGRESS:
      return;
    case AsyncOperationStatus::COMPLETED:
    case AsyncOperationStatus::ERROR:
      adaptor.finish_request_resumable(std::move(pending_response));
      pending_response = nullptr;
      update_state_idle();
      break;
  }
}

void MysqlConnector::handle_special() noexcept {
}

} // namespace database_drivers

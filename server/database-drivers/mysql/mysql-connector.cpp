// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql-connector.h"

#include <mysql/mysql.h>

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/mysql/mysql-request.h"
#include "server/database-drivers/mysql/mysql-response.h"
#include "server/database-drivers/mysql/mysql.h"
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

std::unique_ptr<Response> MysqlConnector::make_response() const noexcept {
  return std::make_unique<MysqlResponse>(connector_id, pending_request->request_id);
}
} // namespace database_drivers

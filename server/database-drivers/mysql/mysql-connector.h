// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <mysql/mysql.h>

#include "runtime/kphp_core.h"
#include "server/database-drivers/connector.h"

namespace database_drivers {

class Request;
class Response;

class MysqlConnector final : public Connector {
public:
  MYSQL *ctx{};

  MysqlConnector(MYSQL *ctx, string host, string user, string password, string db_name, int port);

  ~MysqlConnector() noexcept final;

  AsyncOperationStatus connect_async() noexcept final;

  int get_fd() const noexcept final;

  void push_async_request(std::unique_ptr<Request> &&request) noexcept final;
private:
  string host{};
  string user{};
  string password{};
  string db_name{};
  int port{};
  std::unique_ptr<Request> pending_request;
  std::unique_ptr<Response> pending_response;

  void handle_read() noexcept override;
  void handle_write() noexcept override;
  void handle_special() noexcept override;
};

} // namespace database_drivers

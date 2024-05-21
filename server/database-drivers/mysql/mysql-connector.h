// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <mysql/mysql.h>

#include "kphp-core/kphp_core.h"
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

private:
  string host{};
  string user{};
  string password{};
  string db_name{};
  int port{};

  std::unique_ptr<Response> make_response() const noexcept override;
};

} // namespace database_drivers

// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mysql/mysql.h>

#include "server/database-drivers/response.h"

namespace database_drivers {

class MysqlConnector;

class MysqlResponse final : public Response {
public:
  MYSQL_RES *res{nullptr};
  uint64_t affected_rows{0};
  bool is_error{false};

  using Response::Response;

  AsyncOperationStatus fetch_async() noexcept final;

  ~MysqlResponse() final;

private:
  bool got_columns_info{false};
};

} // namespace database_drivers

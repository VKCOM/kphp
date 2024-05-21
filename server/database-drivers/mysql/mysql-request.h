// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/kphp_core.h"
#include "server/database-drivers/request.h"

namespace database_drivers {

class MysqlConnector;

class MysqlRequest final : public Request {
public:
  MysqlRequest(int connector_id, const string &request);

  AsyncOperationStatus send_async() noexcept final;

private:
  string request;
};

} // namespace database_drivers

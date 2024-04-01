// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql-request.h"

#include <mysql/mysql.h>

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/mysql/mysql-connector.h"
#include "server/database-drivers/mysql/mysql.h"

namespace database_drivers {

MysqlRequest::MysqlRequest(int connector_id, const string &request)
  : Request(connector_id)
  , request(request) {}

AsyncOperationStatus MysqlRequest::send_async() noexcept {
  dl::CriticalSectionGuard guard;

  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<MysqlConnector>(connector_id);
  if (connector == nullptr) {
    return AsyncOperationStatus::ERROR;
  }
  assert(connector->connected());
  tvkprintf(mysql, 1, "MySQL send request: request_id = %d\n", request_id);
  int error = LIB_MYSQL_CALL(mysql_send_query(connector->ctx, request.c_str(), request.size()));
  if (error != 0) {
    return AsyncOperationStatus::ERROR;
  } else {
    return AsyncOperationStatus::COMPLETED;
  }
}

} // namespace database_drivers

// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "server/external-net-drivers/mysql/mysql-response.h"
#include "runtime/pdo/mysql/mysql_pdo.h"
#include "runtime/php_assert.h"

bool MysqlResponse::fetch_async() noexcept {
  auto *connector = vk::singleton<NetDriversAdaptor>::get().get_connector<MysqlConnector>(connector_id);
  if (connector == nullptr) {
    // TODO: message
    return false;
  }
  if (!got_columns_info) {
    LIB_MYSQL_CALL(mysql_read_query_result(connector->ctx));
    tvkprintf(mysql, 1, "MySQL fetch response: get columns info, request_id = %d\n", bound_request_id);
    got_columns_info = true;
  }
  net_async_status status = LIB_MYSQL_CALL(mysql_store_result_nonblocking(connector->ctx, &res)); // buffered queries
  tvkprintf(mysql, 1, "MySQL fetch response: get result set, request_id = %d, status = %d\n", bound_request_id, status);
  return status == NET_ASYNC_COMPLETE;
}

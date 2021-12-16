// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>

#include "server/external-net-drivers/connector.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/external-net-drivers/mysql/mysql-request.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "runtime/pdo/mysql/mysql_pdo.h"

MysqlRequest::MysqlRequest(int connector_id, const string &request)
  : Request(connector_id)
  , request(request) {}

bool MysqlRequest::send_async() noexcept {
  auto *connector = vk::singleton<NetDriversAdaptor>::get().get_connector<MysqlConnector>(connector_id);
  if (connector == nullptr) {
    // TODO: message
    return false;
  }
  assert(connector->connected());
  tvkprintf(mysql, 1, "MySQL send request: request_id = %d\n", request_id);
  LIB_MYSQL_CALL(mysql_send_query(connector->ctx, request.c_str(), request.size()));
  return true;
}

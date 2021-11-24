// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>

#include "server/external-net-drivers/connector.h"
#include "server/external-net-drivers/mysql/mysql-request.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "runtime/pdo/mysql/mysql_pdo.h"

MysqlRequest::MysqlRequest(MysqlConnector *connector, const string &request)
  : connector(connector)
  , request(request) {}

bool MysqlRequest::send_async() noexcept {
  assert(connector->connected());
  LIB_MYSQL_CALL(mysql_send_query(connector->ctx, request.c_str(), request.size()));
  return true;
}

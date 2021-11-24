// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/external-net-drivers/mysql/mysql-response.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "runtime/pdo/mysql/mysql_pdo.h"
#include "runtime/php_assert.h"

bool MysqlResponse::fetch() noexcept {
  LIB_MYSQL_CALL(mysql_read_query_result(connector->ctx));
  net_async_status status = LIB_MYSQL_CALL(mysql_store_result_nonblocking(connector->ctx, &res)); // buffered queries
  php_assert(status == NET_ASYNC_COMPLETE);
  return true;
}

MysqlResponse::MysqlResponse(MysqlConnector *connector)
  : connector(connector) {}

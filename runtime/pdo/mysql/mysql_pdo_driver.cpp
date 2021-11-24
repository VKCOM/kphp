// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include <mysql/mysql.h>

#include "runtime/array_functions.h"
#include "runtime/pdo/mysql/mysql_pdo.h"
#include "runtime/pdo/mysql/mysql_pdo_driver.h"
#include "runtime/pdo/mysql/mysql_pdo_statement.h"
#include "runtime/pdo/pdo.h"
#include "runtime/pdo/pdo_statement.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"
#include "server/php-queries.h"

namespace pdo::mysql {

void MysqlPdoDriver::connect(const class_instance<C$PDO> &pdo_instance, const string &connection_string, const Optional<string> &username,
                             const Optional<string> &password, const Optional<array<mixed>> &options) noexcept {
  (void) pdo_instance, (void)options; // TODO: use options
  array<string> connection_string_parts = explode(';', connection_string);

  string host;
  string db_name;
  int port = 0;

  for (const auto &it : connection_string_parts) {
    array<string> kv_parts = explode('=', it.get_value());
    php_assert(kv_parts.count() == 2);
    const auto &key = kv_parts[0];
    const auto &value = kv_parts[1];

    if (key == string{"host"}) {
      host = value;
    } else if (key == string{"dbname"}) {
      db_name = value;
    } else if (key == string{"port"}) {
      port = static_cast<int>(value.to_int());
    } else {
      php_critical_error("Wrong dsn key %s at MySQL PDO::__construct", key.c_str());
    }
  }

  MYSQL *ctx = LIB_MYSQL_CALL(mysql_init(nullptr));

  Connector *connector = new MysqlConnector{ctx, host, username.val(), password.val(), db_name, port};
  php_query_connect_answer_t *ans = vk::singleton<NetDriversAdaptor>::get().php_query_connect(connector);
  connection_id = ans->connection_id;
}

class_instance<C$PDOStatement> MysqlPdoDriver::prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept {
  (void)options;
  class_instance<C$PDOStatement> res;
  res.alloc();

  int connection_id = v$this.get()->driver->connection_id;
  MysqlConnector *connector = vk::singleton<NetDriversAdaptor>::get().get_connector<MysqlConnector>(connection_id);

  auto *statement = new MysqlPdoEmulatedStatement{connector, query};
  res.get()->statement = statement;

  return res;
}

} // namespace pdo::mysql

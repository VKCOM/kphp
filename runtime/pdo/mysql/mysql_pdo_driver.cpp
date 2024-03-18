// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <memory>
#include <mysql/mysql.h>

#include "runtime/array_functions.h"
#include "runtime/pdo/mysql/mysql_pdo_driver.h"
#include "runtime/pdo/mysql/mysql_pdo_emulated_statement.h"
#include "runtime/pdo/pdo.h"
#include "runtime/pdo/pdo_statement.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/mysql/mysql-connector.h"
#include "server/database-drivers/mysql/mysql-response.h"
#include "server/database-drivers/mysql/mysql.h"
#include "server/php-queries.h"

namespace pdo::mysql {

void MysqlPdoDriver::connect(const class_instance<C$PDO> &v$this, const string &connection_string, const Optional<string> &username,
                             const Optional<string> &password, const Optional<array<mixed>> &options) noexcept {
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

  if (options.has_value()) {
    for (auto it = options.val().cbegin(); it != options.val().cend(); ++it) {
      switch (int64_t option = it.get_int_key()) {
        case C$PDO::ATTR_TIMEOUT: {
          int64_t timeout_sec = it.get_value().to_int();
          v$this.get()->timeout_sec = timeout_sec;
          int error = LIB_MYSQL_CALL(mysql_options(ctx, MYSQL_OPT_CONNECT_TIMEOUT, &timeout_sec));
          if (error) {
            php_warning("Can't set MySQL PDO::ATTR_TIMEOUT option with value = %" PRId64, timeout_sec);
          }
          break;
        }
        default: {
          php_warning("MySQL option %" PRId64 " is not supported", option);
        }
      }
    }
  }

  std::unique_ptr<database_drivers::Connector> connector =
    std::make_unique<database_drivers::MysqlConnector>(ctx, host, username.val(), password.val(), db_name, port);
  connector_id = vk::singleton<database_drivers::Adaptor>::get().initiate_connect(std::move(connector));
}

class_instance<C$PDOStatement> MysqlPdoDriver::prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept {
  (void)options;
  class_instance<C$PDOStatement> res;
  res.alloc();

  res.get()->statement = std::make_unique<MysqlPdoEmulatedStatement>(query, v$this.get()->driver->connector_id);
  res.get()->timeout_sec = v$this->timeout_sec;

  return res;
}

const char *MysqlPdoDriver::error_code_sqlstate() noexcept {
  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<database_drivers::MysqlConnector>(connector_id);
  if (connector == nullptr) {
    return nullptr;
  }
  return LIB_MYSQL_CALL(mysql_sqlstate(connector->ctx));
}

std::pair<int, const char *> MysqlPdoDriver::error_info() noexcept {
  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<database_drivers::MysqlConnector>(connector_id);
  if (connector == nullptr) {
    return {};
  }

  MYSQL *mysql = connector->ctx;
  return {LIB_MYSQL_CALL(mysql_errno(mysql)), LIB_MYSQL_CALL(mysql_error(mysql))};
}

} // namespace pdo::mysql

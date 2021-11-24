// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>


#include "runtime/pdo/pdo.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/pdo/mysql/mysql_pdo.h"
#include "runtime/pdo/mysql/mysql_pdo_driver.h"
#include "runtime/array_functions.h"

namespace pdo {
void init_lib() {
  mysql::init_lib();
}
void free_lib() {
  mysql::free_lib();
}
} // namespace pdo


class_instance<C$PDO> f$PDO$$__construct(const class_instance<C$PDO> &v$this, const string &dsn,
                                         const Optional<string> &username, const Optional<string> &password, const Optional<array<mixed>> &options) noexcept {
  array<string> dsn_parts = explode(':', dsn);
  php_assert(dsn_parts.count() == 2);
  const auto &driver_name = dsn_parts[0];
  const auto &connection_string = dsn_parts[1];

  if (driver_name == string{"mysql"}) {
    v$this.get()->driver = new pdo::mysql::MysqlPdoDriver{};
  } else {
    php_critical_error("Unknown PDO driver name: %s", driver_name.c_str());
  }

  v$this.get()->driver->connect(v$this, connection_string, username, password, options);

  return v$this;
}

class_instance<C$PDOStatement> f$PDO$$prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept {
  return v$this.get()->driver->prepare(v$this, query, options);
}

class_instance<C$PDOStatement> f$PDO$$query(const class_instance<C$PDO> &v$this, const string &query, Optional<int64_t> fetchMode) {
  (void)fetchMode;
  class_instance<C$PDOStatement> statement = f$PDO$$prepare(v$this, query);
  bool ok = f$PDOStatement$$execute(statement);
  if (!ok) {
    return {};
  }
  return statement;
}

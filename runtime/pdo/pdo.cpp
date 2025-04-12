// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt


#include "runtime/array_functions.h"
#include "runtime/pdo/pdo.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/pdo/mysql/mysql_pdo_driver.h"
#include "runtime/pdo/pgsql/pgsql_pdo_driver.h"
#include "runtime/resumable.h"


class_instance<C$PDO> f$PDO$$__construct(const class_instance<C$PDO> &v$this, const string &dsn,
                                         const Optional<string> &username, const Optional<string> &password, const Optional<array<mixed>> &options) noexcept {
  array<string> dsn_parts = explode(':', dsn);
  php_assert(dsn_parts.count() == 2);
  const auto &driver_name = dsn_parts[0];
  const auto &connection_string = dsn_parts[1];

  if (driver_name == string{"mysql"}) {
#ifdef PDO_DRIVER_MYSQL
    v$this.get()->driver = std::make_unique<pdo::mysql::MysqlPdoDriver>();
#else
    php_critical_error("PDO MySQL driver is disabled");
#endif
  } else if (driver_name == string{"pgsql"}) {
#ifdef PDO_DRIVER_PGSQL
    v$this.get()->driver = std::make_unique<pdo::pgsql::PgsqlPdoDriver>();
#else
    php_critical_error("PDO pgSQL driver is disabled");
#endif
  }
  else {
    php_critical_error("Unknown PDO driver name: %s", driver_name.c_str());
  }

  v$this.get()->driver->connect(v$this, connection_string, username, password, options);

  return v$this;
}

class_instance<C$PDOStatement> f$PDO$$prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept {
  // TODO: prepared statements are not supported so far
  return v$this.get()->driver->prepare(v$this, query, options);
}

class PdoQueryResumable final : public Resumable {
private:
  const class_instance<C$PDO> &v$this;
  const string &query;

  class_instance<C$PDOStatement> statement;
  bool ok{};
public:
  using ReturnT = class_instance<C$PDOStatement>;

  explicit PdoQueryResumable(const class_instance<C$PDO> &v$this, const string &query) noexcept : v$this(v$this), query(query)  {}

  bool run() noexcept final {
    RESUMABLE_BEGIN
      statement = f$PDO$$prepare(v$this, query);
      ok = f$PDOStatement$$execute(statement);
      TRY_WAIT(PdoQueryResumable_label, ok, bool);
      if (!ok) {
        RETURN({});
      }
      RETURN(statement);
    RESUMABLE_END
  }
};

class PdoExecResumable final : public Resumable {
private:
  PdoQueryResumable *query;

  class_instance<C$PDOStatement> statement;
public:
  using ReturnT = Optional<int64_t>;

  explicit PdoExecResumable(const class_instance<C$PDO> &v$this, const string &query) noexcept
    : query(new PdoQueryResumable(v$this, query))  {}

  bool run() {
    RESUMABLE_BEGIN
      statement = start_resumable<class_instance<C$PDOStatement>>(query);
      TRY_WAIT(PdoExecResumable_label, statement, class_instance<C$PDOStatement>)
      if (statement.is_null()) {
        RETURN(false);
      }
      RETURN(statement.get()->statement->affected_rows());
    RESUMABLE_END
  }
};

class_instance<C$PDOStatement> f$PDO$$query(const class_instance<C$PDO> &v$this, const string &query, Optional<int64_t>) {
  return start_resumable<class_instance<C$PDOStatement>>(new PdoQueryResumable{v$this, query});
}

Optional<int64_t> f$PDO$$exec(const class_instance<C$PDO> &v$this, const string &query) {
  return start_resumable<Optional<int64_t>>(new PdoExecResumable{v$this, query});
}

Optional<string> f$PDO$$errorCode(const class_instance<C$PDO> &v$this) {
  const char *sqlstate = v$this.get()->driver->error_code_sqlstate();
  if (sqlstate == nullptr) {
    return {};
  }
  return string{sqlstate};
}

array<mixed> f$PDO$$errorInfo(const class_instance<C$PDO> &v$this) {
  auto sqlstate = f$PDO$$errorCode(v$this);
  if (!sqlstate.has_value()) {
    return {};
  }
  auto [error_code, error_msg] = v$this.get()->driver->error_info();

  array<mixed> res(array_size{3, true});
  res[0] = sqlstate.val();
  res[1] = error_code == 0 ? mixed() : error_code;
  res[2] = strcmp(error_msg, "") == 0 ? mixed() : string{error_msg};
  return res;
}

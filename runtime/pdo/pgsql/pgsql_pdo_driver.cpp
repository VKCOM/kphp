#include <memory>
#include <postgresql/libpq-fe.h>

#include "runtime/array_functions.h"
#include "runtime/pdo/pdo.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/pdo/pgsql/pgsql_pdo_driver.h"
#include "runtime/pdo/pgsql/pgsql_pdo_emulated_statement.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/pgsql/pgsql-connector.h"
#include "server/database-drivers/pgsql/pgsql-response.h"
#include "server/php-queries.h"

namespace pdo::pgsql {

void PgsqlPdoDriver::connect(const class_instance<C$PDO>& v$this __attribute__((unused)), const string& connection_string, const Optional<string>& username,
                             const Optional<string>& password, const Optional<array<mixed>>& options) noexcept {
  array<string> connection_string_parts = explode(';', connection_string);
  string conninfo{};

  for (const auto& it : connection_string_parts) {
    array<string> kv_parts = explode('=', it.get_value());
    php_assert(kv_parts.count() == 2);
    const auto& key = kv_parts[0];
    const auto& value = kv_parts[1];

    if (key == string{"host"}) {
      conninfo.append(" host=").append(value.c_str());
    } else if (key == string{"dbname"}) {
      conninfo.append(" dbname=").append(value.c_str());
    } else if (key == string{"port"}) {
      conninfo.append(" port=").append(value.c_str());
    } else {
      php_critical_error("Wrong dsn key %s at pgSQL PDO::__construct", key.c_str());
    }
  }
  if (username.has_value()) {
    conninfo.append(" user=").append(username.val());
  }
  if (password.has_value()) {
    conninfo.append(" password=").append(password.val());
  }

  if (options.has_value()) {
    for (auto it = options.val().cbegin(); it != options.val().cend(); ++it) {
      switch (int64_t option = it.get_int_key()) {
      case C$PDO::ATTR_TIMEOUT: {
        int64_t timeout_sec = it.get_value().to_int();
        v$this.get()->timeout_sec = timeout_sec;
        conninfo.append(" connect_timeout=").append(string(timeout_sec));
        break;
      }
      default: {
        php_warning("pgSQL option %" PRId64 " is not supported", option);
      }
      }
    }
  }

  std::unique_ptr<database_drivers::Connector> connector = std::make_unique<database_drivers::PgsqlConnector>(std::move(conninfo));
  connector_id = vk::singleton<database_drivers::Adaptor>::get().initiate_connect(std::move(connector));
}

class_instance<C$PDOStatement> PgsqlPdoDriver::prepare(const class_instance<C$PDO>& v$this, const string& query, const array<mixed>& options) noexcept {
  (void)options;
  class_instance<C$PDOStatement> res;
  res.alloc();

  res.get()->statement = std::make_unique<PgsqlPdoEmulatedStatement>(query, v$this.get()->driver->connector_id);
  res.get()->timeout_sec = v$this->timeout_sec;

  return res;
}

const char* PgsqlPdoDriver::error_code_sqlstate() noexcept {
  auto* connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<database_drivers::PgsqlConnector>(connector_id);
  if (connector == nullptr) {
    return {};
  }
  string code = connector->ctx.sqlstate;
  return code.empty() ? "00000" : code.c_str();
}

std::pair<int, const char*> PgsqlPdoDriver::error_info() noexcept {
  auto* connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<database_drivers::PgsqlConnector>(connector_id);
  if (connector == nullptr) {
    return {};
  }
  int status = connector->ctx.last_result_status;
  char* message = LIB_PGSQL_CALL(PQerrorMessage(connector->ctx.conn));
  return {status == 1 || status == 2 ? 0 : status, message};
}

} // namespace pdo::pgsql

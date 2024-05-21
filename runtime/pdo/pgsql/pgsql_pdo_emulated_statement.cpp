#include <postgresql/libpq-fe.h>

#include "kphp-core/kphp_core.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/pdo/pgsql/pgsql_pdo_emulated_statement.h"
#include "runtime/resumable.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/pgsql/pgsql-request.h"
#include "server/database-drivers/pgsql/pgsql-response.h"
#include "server/database-drivers/pgsql/pgsql.h"

namespace pdo::pgsql {

class PgsqlPdoEmulatedStatement::ExecuteResumable final : public Resumable {
private:
  PgsqlPdoEmulatedStatement *ctx{};
  int64_t timeout_sec{-1};
  std::unique_ptr<database_drivers::Response> response{};
  int resumable_id{};

public:
  using ReturnT = bool;
  explicit ExecuteResumable(PgsqlPdoEmulatedStatement *ctx, int64_t timeout_sec) noexcept
    : ctx(ctx)
    , timeout_sec(timeout_sec) {}
  bool run() noexcept final {
    RESUMABLE_BEGIN
      resumable_id = vk::singleton<database_drivers::Adaptor>::get().launch_request_resumable(
        std::make_unique<database_drivers::PgsqlRequest>(ctx->connector_id, ctx->statement));
      response = vk::singleton<database_drivers::Adaptor>::get().wait_request_resumable(resumable_id, timeout_sec);
      TRY_WAIT(PgsqlPdoEmulatedStatement_ExecuteResumable_label, response, std::unique_ptr<database_drivers::Response>);
      if (auto *casted = dynamic_cast<database_drivers::PgsqlResponse *>(response.get())) {
        ctx->response = std::unique_ptr<database_drivers::PgsqlResponse>{casted};
        response.release();
      } else {
        php_critical_error("Unexpected error at pgSQL PDO::execute");
      }
      RETURN(!ctx->response->is_error);
    RESUMABLE_END
  }
};

PgsqlPdoEmulatedStatement::PgsqlPdoEmulatedStatement(const string &statement, int connector_id)
  : statement(statement)
  , connector_id(connector_id) {}

bool PgsqlPdoEmulatedStatement::execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  (void)params;
  return start_resumable<bool>(new ExecuteResumable(this, v$this.get()->timeout_sec));
}

mixed PgsqlPdoEmulatedStatement::fetch(const class_instance<C$PDOStatement> &) noexcept {
  PGresult *pGresult = response->res;
  assert(LIB_PGSQL_CALL(PQresultStatus(pGresult)) == PGRES_TUPLES_OK || PQresultStatus(pGresult) == PGRES_COMMAND_OK);
  if (LIB_PGSQL_CALL(PQresultStatus(pGresult)) == PGRES_COMMAND_OK) {
    return array<mixed>();
  } else if (processed_row + 1 == LIB_PGSQL_CALL(PQntuples(pGresult))) {
    return false;
  }

  ++processed_row;
  int columns = LIB_PGSQL_CALL(PQnfields(pGresult));
  array<mixed> res;
  {
    dl::CriticalSectionGuard guard;
    for (int column = 0; column < columns; ++column) {
      res.set_value(string{PQfname(pGresult, column)}, string{PQgetvalue(pGresult, processed_row, column)});
      res.set_value(column, string{PQgetvalue(pGresult, processed_row, column)});
    }
  }
  return res;
}

int64_t PgsqlPdoEmulatedStatement::affected_rows() noexcept {
  return response->affected_rows;
}
} // namespace pdo::pgsql

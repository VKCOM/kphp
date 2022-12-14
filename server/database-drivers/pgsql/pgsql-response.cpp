#include "server/database-drivers/pgsql/pgsql-response.h"

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/pgsql/pgsql-connector.h"
#include "server/database-drivers/pgsql/pgsql-resources.h"
#include "server/database-drivers/pgsql/pgsql.h"

namespace database_drivers {

AsyncOperationStatus PgsqlResponse::fetch_async() noexcept {
  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<PgsqlConnector>(connector_id);
  if (connector == nullptr) {
    is_error = true;
    return AsyncOperationStatus::ERROR;
  }

  dl::CriticalSectionGuard guard;
  ConnStatusType status = LIB_PGSQL_CALL(PQstatus(connector->ctx.conn));
  tvkprintf(pgsql, 1, "pgSQL fetch response: request_id = %d, get result set, status = %d\n", bound_request_id, status);
  switch (status) {
    case CONNECTION_OK:
      res = LIB_PGSQL_CALL(PQgetResult(connector->ctx.conn));
      assert(res != nullptr);
      connector->ctx.remember_result_status(res);
      register_pgsql_response(res);
      while (LIB_PGSQL_CALL(PQgetResult(connector->ctx.conn)) != nullptr) {
        // PQgetResult must be called repeatedly until it returns a null pointer, indicating that the command is done.
      }
      if (LIB_PGSQL_CALL(PQresultStatus(res)) == PGRES_TUPLES_OK || LIB_PGSQL_CALL(PQresultStatus(res)) == PGRES_COMMAND_OK) {
        affected_rows = LIB_PGSQL_CALL(string{(PQcmdTuples(res))}.to_int());
        return AsyncOperationStatus::COMPLETED;
      } else {
        is_error = true;
        return AsyncOperationStatus::ERROR;
      }
    case CONNECTION_AWAITING_RESPONSE:
      return AsyncOperationStatus::IN_PROGRESS;
    case CONNECTION_BAD:
    default:
      is_error = true;
      return AsyncOperationStatus::ERROR;
  }
}

PgsqlResponse::~PgsqlResponse() {
  if (res) {
    remove_pgsql_response(res);
  }
}

} // namespace database_drivers

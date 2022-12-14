#include "server/database-drivers/pgsql/pgsql-request.h"

#include <postgresql/libpq-fe.h>

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/pgsql/pgsql-connector.h"

namespace database_drivers {

PgsqlRequest::PgsqlRequest(int connector_id, const string &request)
  : Request(connector_id)
  , request(request) {}

AsyncOperationStatus PgsqlRequest::send_async() noexcept {
  dl::CriticalSectionGuard guard;

  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<PgsqlConnector>(connector_id);
  if (connector == nullptr) {
    return AsyncOperationStatus::ERROR;
  }
  assert(connector->connected());
  tvkprintf(pgsql, 1, "pgSQL send request: request_id = %d\n", request_id);
  int status = LIB_PGSQL_CALL(PQsendQuery(connector->ctx.conn, request.c_str()));
  if (status != 1) {
    return AsyncOperationStatus::ERROR;
  } else {
    return AsyncOperationStatus::COMPLETED;
  }
}

} // namespace database_drivers

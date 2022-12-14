#include "server/database-drivers/pgsql/pgsql-connector.h"

#include <postgresql/libpq-fe.h>

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/pgsql/pgsql-response.h"
#include "server/database-drivers/pgsql/pgsql.h"
#include "server/php-engine.h"

namespace database_drivers {

PgsqlConnector::PgsqlConnector(string conninfo)
  : ctx()
  , conninfo(std::move(conninfo)) {}

PgsqlConnector::~PgsqlConnector() noexcept {
  if (is_connected) {
    epoll_remove(get_fd());
    tvkprintf(pgsql, 1, "pgSQL disconnected from [%s:%d]: connector_id = %d\n", LIB_PGSQL_CALL(PQhost(ctx.conn)),
              (int)string{LIB_PGSQL_CALL(PQport(ctx.conn))}.to_int(), connector_id);
    LIB_PGSQL_CALL(PQfinish(ctx.conn));
  }
}

int PgsqlConnector::get_fd() const noexcept {
  if (!is_connected) {
    return -1;
  }
  return LIB_PGSQL_CALL(PQsocket(ctx.conn));
}

AsyncOperationStatus PgsqlConnector::connect_async() noexcept {
  if (is_connected) {
    return AsyncOperationStatus::COMPLETED;
  }

  if (ctx.conn == nullptr) {
    if ((ctx.conn = LIB_PGSQL_CALL(PQconnectStart(conninfo.c_str()))) == nullptr) {
      return AsyncOperationStatus::ERROR;
    }
  }

  PostgresPollingStatusType status = LIB_PGSQL_CALL(PQconnectPoll(ctx.conn));
  tvkprintf(pgsql, 1, "pgSQL try to connect to [%s:%d]: connector_id = %d, status = %d\n", LIB_PGSQL_CALL(PQhost(ctx.conn)),
            (int)string{LIB_PGSQL_CALL(PQport(ctx.conn))}.to_int(), connector_id, status);
  switch (status) {
    case PGRES_POLLING_OK:
      return AsyncOperationStatus::COMPLETED;
    case PGRES_POLLING_READING:
    case PGRES_POLLING_WRITING:
      return AsyncOperationStatus::IN_PROGRESS;
    case PGRES_POLLING_FAILED:
    default:
      // todo Are we need to throw exception?
      return AsyncOperationStatus::ERROR;
  }
}

std::unique_ptr<Response> PgsqlConnector::make_response() const noexcept {
  return std::make_unique<PgsqlResponse>(connector_id, pending_request->request_id);
}
} // namespace database_drivers

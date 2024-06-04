#pragma once

#include <memory>
#include <postgresql/libpq-fe.h>

#include "runtime/runtime-types.h"
#include "server/database-drivers/connector.h"
#include "server/database-drivers/pgsql/pgsql.h"
#include "server/database-drivers/pgsql/pgsql-storage.h"

namespace database_drivers {

class Request;
class Response;

class PgsqlConnector final : public Connector {
public:
  PGSQL ctx{};

  PgsqlConnector(string conninfo);

  ~PgsqlConnector() noexcept final;

  AsyncOperationStatus connect_async() noexcept final;

  int get_fd() const noexcept final;

private:
  string conninfo{};

  std::unique_ptr<Response> make_response() const noexcept override;
};

} // namespace database_drivers

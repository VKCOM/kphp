#pragma once

#include <memory>
#include <postgresql/libpq-fe.h>

#include "runtime/kphp_core.h"
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

  void push_async_request(std::unique_ptr<Request> &&request) noexcept final;

private:
  string conninfo{};
  std::unique_ptr<Request> pending_request;
  std::unique_ptr<Response> pending_response;

  void handle_read() noexcept override;
  void handle_write() noexcept override;
  void handle_special() noexcept override;
};

} // namespace database_drivers

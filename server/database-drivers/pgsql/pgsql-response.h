#pragma once

#include <postgresql/libpq-fe.h>

#include "server/database-drivers/response.h"

namespace database_drivers {

class PgsqlConnector;

class PgsqlResponse final : public Response {
public:
  PGresult *res{nullptr};
  uint64_t affected_rows{0};
  bool is_error{false};

  using Response::Response;

  AsyncOperationStatus fetch_async() noexcept final;

  ~PgsqlResponse() final;
};

} // namespace database_drivers

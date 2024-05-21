#pragma once

#include "kphp-core/kphp_core.h"
#include "server/database-drivers/request.h"

namespace database_drivers {

class PgsqlConnector;

class PgsqlRequest final : public Request {
public:
  PgsqlRequest(int connector_id, const string &request);

  AsyncOperationStatus send_async() noexcept final;

private:
  string request;
};
} // namespace database_drivers

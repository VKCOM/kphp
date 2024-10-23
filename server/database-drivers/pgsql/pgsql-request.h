#pragma once

#include "runtime-common/runtime-core/runtime-core.h"
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

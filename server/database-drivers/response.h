// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"
#include "server/database-drivers/async-operation-status.h"

namespace database_drivers {

class Response : public ManagedThroughDlAllocator {
public:
  int connector_id{};
  int bound_request_id{};

  Response(int connector_id, int bound_request_id)
    : connector_id(connector_id)
    , bound_request_id(bound_request_id) {}

  /**
   * @brief Fetches this response asynchronously.
   * @return Status of operation: in progress, completed or error.
   */
  virtual AsyncOperationStatus fetch_async() noexcept = 0;

  virtual ~Response() noexcept = default;
};

} // namespace database_drivers

// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"
#include "server/database-drivers/async-operation-status.h"

namespace database_drivers {

class Request : public ManagedThroughDlAllocator {
public:
  int connector_id{};
  int request_id{};

  explicit Request(int connector_id)
    : connector_id(connector_id) {}

  /**
   * @brief Sends this requests asynchronously.
   * @return Status of operation: in progress, completed or error.
   */
  virtual AsyncOperationStatus send_async() noexcept = 0;

  virtual ~Request() noexcept = default;
};

} // namespace database_drivers

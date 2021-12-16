// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"

class Request : public ManagedThroughDlAllocator {
public:
  int connector_id{};
  int request_id{};

  explicit Request(int connector_id) : connector_id(connector_id) {}

  virtual bool send_async() noexcept = 0;
  virtual ~Request() noexcept = default;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"

class Response : public ManagedThroughDlAllocator {
public:
  int connector_id{};
  int bound_request_id{};

  explicit Response(int connector_id) : connector_id(connector_id) {}

  virtual bool fetch_async() noexcept = 0;
  virtual ~Response() noexcept = default;
};

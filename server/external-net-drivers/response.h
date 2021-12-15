// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"

class Response : public ManagedThroughDlAllocator {
public:
  int bound_request_id{};

  virtual bool fetch_async() noexcept = 0;
};

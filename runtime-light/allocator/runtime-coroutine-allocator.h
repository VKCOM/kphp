// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/details/pool-allocator.h"

struct RuntimeCoroutineAllocator final : public kphp::memory::details::PoolAllocator {
  using kphp::memory::details::PoolAllocator::PoolAllocator;

  static auto get() noexcept -> RuntimeCoroutineAllocator&;
};

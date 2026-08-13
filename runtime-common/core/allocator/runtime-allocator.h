//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/details/pool-allocator.h"

struct RuntimeAllocator final : public kphp::memory::details::PoolAllocator {
  using kphp::memory::details::PoolAllocator::PoolAllocator;

  static RuntimeAllocator& get() noexcept;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"

class AllocationsStorage {
  kphp::stl::stack<kphp::stl::unordered_set<void*, kphp::memory::script_allocator>, kphp::memory::script_allocator> m_spans;

public:
  void register_allocation(void* allocation) noexcept;
  void unregister_allocation(void* allocation) noexcept;

  void push_span() noexcept;
  void pop_span();

  static const AllocationsStorage& get() noexcept;
  static AllocationsStorage& get_mutable() noexcept;
};

struct AllocationsSpan {
  AllocationsSpan();
  ~AllocationsSpan();
};

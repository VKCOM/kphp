// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"

class AllocatorState final : private vk::not_copyable {
  uint32_t m_libc_alloc_allowed{};

public:
  RuntimeAllocator allocator;

  void enable_libc_alloc() noexcept {
    ++m_libc_alloc_allowed;
  }
  void disable_libc_alloc() noexcept {
    kphp::log::assertion(m_libc_alloc_allowed-- != 0);
  }
  bool libc_alloc_allowed() const noexcept {
    return m_libc_alloc_allowed != 0;
  }

  AllocatorState(size_t script_mem_size, size_t oom_handling_mem_size) noexcept
      : allocator(script_mem_size, oom_handling_mem_size) {}

  static const AllocatorState& get() noexcept;
  static AllocatorState& get_mutable() noexcept;
};

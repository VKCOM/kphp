//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/details/pool-allocator.h"

struct RuntimeAllocator final {
private:
  kphp::memory::details::pool_allocator m_allocator;

public:
  static auto get() noexcept -> RuntimeAllocator&;

  RuntimeAllocator() = default;
  RuntimeAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept;

  auto init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void;
  auto free() noexcept -> void;

  auto alloc_script_memory(size_t size) noexcept -> void*;
  auto alloc0_script_memory(size_t size) noexcept -> void*;
  auto realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_script_memory(void* mem, size_t size) noexcept -> void;
};

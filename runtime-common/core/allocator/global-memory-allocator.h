//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"

struct GlobalMemoryAllocator final : private vk::not_copyable {
  static auto get() noexcept -> GlobalMemoryAllocator&;

  auto alloc_global_memory(size_t size) noexcept -> void*;
  auto alloc0_global_memory(size_t size) noexcept -> void*;
  auto realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_global_memory(void* mem, size_t size) noexcept -> void;
};

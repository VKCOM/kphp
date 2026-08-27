//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"

namespace kphp::memory {

struct pool_allocator : private vk::not_copyable {
private:
  memory_resource::unsynchronized_pool_resource memory_resource;
  size_t m_min_extra_mem_size{0};

  auto request_extra_memory(size_t requested_size) noexcept -> void;

public:
  pool_allocator() noexcept = default;
  pool_allocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept;

  auto init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void;
  auto free() noexcept -> void;

  auto alloc_script_memory(size_t size) noexcept -> void*;
  auto calloc_script_memory(size_t size) noexcept -> void*;
  auto realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_script_memory(void* mem, size_t size) noexcept -> void;

  auto get_memory_resource() noexcept -> memory_resource::unsynchronized_pool_resource& {
    return memory_resource;
  }
};

} // namespace kphp::memory

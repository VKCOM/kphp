// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"

namespace kphp::coro {

struct CoroutineAllocator final : vk::not_copyable {
  static auto get() noexcept -> CoroutineAllocator&;

  CoroutineAllocator() noexcept = default;
  CoroutineAllocator(size_t mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept;

  void init(void* buffer, size_t mem_size, size_t oom_handling_mem_size) noexcept;
  void free() noexcept;

  auto alloc_memory(size_t size) noexcept -> void*;
  auto alloc0_memory(size_t size) noexcept -> void*;
  auto realloc_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_memory(void* mem, size_t size) noexcept -> void;

private:
  auto request_extra_memory(size_t requested_size) noexcept -> void;

public:
  memory_resource::unsynchronized_pool_resource memory_resource;

private:
  size_t m_min_extra_mem_size{0};
};

} // namespace kphp::coro

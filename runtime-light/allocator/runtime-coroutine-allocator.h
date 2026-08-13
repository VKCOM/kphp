// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"

struct RuntimeCoroutineAllocator final : vk::not_copyable {
  static auto get() noexcept -> RuntimeCoroutineAllocator&;

  RuntimeCoroutineAllocator() noexcept = default;
  RuntimeCoroutineAllocator(size_t mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept;

  auto init(void* buffer, size_t mem_size, size_t oom_handling_mem_size) noexcept -> void;
  auto free() noexcept -> void;

  auto alloc_memory(size_t size) noexcept -> void*;
  auto alloc0_memory(size_t size) noexcept -> void*;
  auto realloc_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_memory(void* mem, size_t size) noexcept -> void;

private:
  auto alloc_global_memory(size_t size) noexcept -> void*;

  auto request_extra_memory(size_t requested_size) noexcept -> void;

public:
  memory_resource::unsynchronized_pool_resource memory_resource;

private:
  size_t m_min_extra_mem_size{0};
};

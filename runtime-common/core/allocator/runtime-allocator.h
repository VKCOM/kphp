//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"

struct RuntimeAllocator final : vk::not_copyable {
  static RuntimeAllocator& get() noexcept;

  RuntimeAllocator() = default;
  RuntimeAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size);

  void init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size);
  void free();

  void* alloc_script_memory(size_t size) noexcept;
  void* alloc0_script_memory(size_t size) noexcept;
  void* realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept;
  void free_script_memory(void* mem, size_t size) noexcept;

  void* alloc_global_memory(size_t size) noexcept;
  void* alloc0_global_memory(size_t size) noexcept;
  void* realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept;
  void free_global_memory(void* mem, size_t size) noexcept;

  [[nodiscard]] std::reference_wrapper<memory_resource::unsynchronized_pool_resource>
  replace_script_memory_resource(memory_resource::unsynchronized_pool_resource& replacement) noexcept {
    return std::exchange(m_script_memory_resource, std::ref(replacement));
  }
  memory_resource::unsynchronized_pool_resource& current_script_memory_resource() const noexcept {
    return m_script_memory_resource.get();
  }

public:
  memory_resource::unsynchronized_pool_resource memory_resource;

private:
  std::reference_wrapper<memory_resource::unsynchronized_pool_resource> m_script_memory_resource{memory_resource};
  size_t m_min_extra_mem_size{0};
};

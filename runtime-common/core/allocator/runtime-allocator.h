// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#ifdef RUNTIME_LIGHT
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/pool-allocator.h"
#endif

struct RuntimeAllocator final {
#ifdef RUNTIME_LIGHT
private:
  kphp::memory::pool_allocator m_allocator;
  std::reference_wrapper<kphp::memory::pool_allocator> m_allocator_ref{m_allocator};
#endif

public:
  static auto get() noexcept -> RuntimeAllocator&;

#ifdef RUNTIME_LIGHT
  RuntimeAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept;
#else
  RuntimeAllocator() = default;
  auto init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void;
#endif

  auto free() noexcept -> void;

  auto alloc_script_memory(size_t size) noexcept -> void*;
  auto calloc_script_memory(size_t size) noexcept -> void*;
  auto realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void*;
  auto free_script_memory(void* mem, size_t size) noexcept -> void;

#ifdef RUNTIME_LIGHT
  auto get_memory_resource() noexcept -> memory_resource::unsynchronized_pool_resource& {
    return m_allocator_ref.get().get_memory_resource();
  }

  // The callback must run synchronously without yielding. Objects allocated by it
  // may outlive the scope, but later operations that allocate or deallocate their
  // memory must install the same allocator. The replacement must outlive the callback.
  template<typename callback_type,
           std::enable_if_t<std::is_nothrow_invocable_v<callback_type> && std::is_same_v<std::invoke_result_t<callback_type>, void>, int32_t> = 0>
  auto with_allocator(kphp::memory::pool_allocator& replacement, callback_type&& callback) noexcept -> void {
    const auto previous_allocator{std::exchange(m_allocator_ref, std::ref(replacement))};
    const auto restore_allocator{vk::finally([this, previous_allocator]() noexcept { m_allocator_ref = previous_allocator; })};
    std::invoke(std::forward<callback_type>(callback));
  }
#endif
};

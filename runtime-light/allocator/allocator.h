//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-allocator-managed.h"
#include "runtime-light/allocator/allocator-state.h"

template<std::derived_from<kphp::memory::script_allocator_managed> T, typename... Args>
requires std::constructible_from<T, Args...>
auto make_unique_on_script_memory(Args&&... args) noexcept {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

namespace kphp::memory {

// Objects allocated by the callback may outlive it, but every later operation
// that can allocate or deallocate their memory must install the same resource.
// Keeping the operation synchronous and non-throwing prevents the replacement
// itself from surviving a suspension or stack unwind.
template<std::invocable callback_type>
requires std::same_as<std::invoke_result_t<callback_type>, void> && std::is_nothrow_invocable_v<callback_type>
void with_script_memory_resource(memory_resource::unsynchronized_pool_resource& resource, callback_type&& callback) noexcept {
  auto& allocator{RuntimeAllocator::get()};
  const auto previous_resource{allocator.replace_script_memory_resource(resource)};
  const auto restore_resource{vk::finally([&allocator, &resource, previous_resource] noexcept {
    kphp::log::assertion(std::addressof(allocator.current_script_memory_resource()) == std::addressof(resource));
    static_cast<void>(allocator.replace_script_memory_resource(previous_resource.get()));
  })};
  std::invoke(std::forward<callback_type>(callback));
}

struct libc_alloc_guard final {
  libc_alloc_guard() noexcept {
    AllocatorState::get_mutable().enable_libc_alloc();
  }

  ~libc_alloc_guard() {
    AllocatorState::get_mutable().disable_libc_alloc();
  }

  libc_alloc_guard(const libc_alloc_guard&) = delete;
  libc_alloc_guard(libc_alloc_guard&&) = delete;
  libc_alloc_guard& operator=(const libc_alloc_guard&) = delete;
  libc_alloc_guard& operator=(libc_alloc_guard&&) = delete;
};
} // namespace kphp::memory

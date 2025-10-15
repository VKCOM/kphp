//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>

#include "runtime-common/core/allocator/script-allocator-managed.h"
#include "runtime-light/allocator/allocator-state.h"

template<std::derived_from<ScriptAllocatorManaged> T, typename... Args>
requires std::constructible_from<T, Args...>
auto make_unique_on_script_memory(Args&&... args) noexcept {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

namespace kphp::memory {
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

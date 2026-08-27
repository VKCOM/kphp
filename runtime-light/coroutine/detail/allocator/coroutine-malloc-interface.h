// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstring>

#include "runtime-common/core/allocator/details/malloc-interface.h"
#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro::detail::memory {

inline auto alloc(size_t size) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::alloc(size);
}

inline auto alloc_aligned(size_t size, std::align_val_t alignment) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::alloc_aligned(size, alignment);
}

inline auto calloc(size_t num, size_t size) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::calloc(num, size);
}

inline auto free(void* ptr) noexcept -> void {
  kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::free(ptr);
}

inline auto realloc(void* ptr, size_t new_size) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::realloc(ptr, new_size);
}

inline auto strdup(const char* str1) noexcept -> char* {
  return kphp::memory::details::malloc_interface<kphp::coro::instance_state::get_coroutine_allocator>::strdup(str1);
}

} // namespace kphp::coro::detail::memory

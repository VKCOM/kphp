// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/details/malloc-interface.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro::detail::memory {

inline auto alloc(size_t size) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::detail::memory::task_allocator::get>::alloc(size);
}

inline auto alloc_aligned(size_t size, std::align_val_t alignment) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::detail::memory::task_allocator::get>::alloc_aligned(size, alignment);
}

inline auto calloc(size_t num, size_t size) noexcept -> void* {
  return kphp::memory::details::malloc_interface<kphp::coro::detail::memory::task_allocator::get>::calloc(num, size);
}

inline auto free(void* ptr) noexcept -> void {
  kphp::memory::details::malloc_interface<kphp::coro::detail::memory::task_allocator::get>::free(ptr);
}

} // namespace kphp::coro::detail::memory

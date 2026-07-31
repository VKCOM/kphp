// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/coroutine/async-stack.h"

struct CoroutineInstanceState final : private vk::not_copyable {

  CoroutineInstanceState() noexcept = default;

  static CoroutineInstanceState& get() noexcept;

  kphp::coro::async_stack_root coroutine_stack_root;

  RuntimeAllocator coro_allocator{static_cast<size_t>(1024 * 1024 * 8), static_cast<size_t>(1024 * 1024 * 8), 0};
};

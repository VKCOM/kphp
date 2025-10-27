// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"

struct RpcQueueInstanceState {
  kphp::stl::unordered_map<int64_t, int64_t, kphp::memory::script_allocator> m_waiter_forks_to_response;

  RpcQueueInstanceState() noexcept = default;

  static RpcQueueInstanceState& get() noexcept;
};

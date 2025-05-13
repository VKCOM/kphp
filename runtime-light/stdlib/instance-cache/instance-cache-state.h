// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

struct InstanceCacheInstanceState final : private vk::not_copyable {
  kphp::stl::unordered_map<string, mixed, kphp::memory::script_allocator, decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); })>
      request_cache;

  InstanceCacheInstanceState() noexcept = default;
  static InstanceCacheInstanceState& get() noexcept;
};

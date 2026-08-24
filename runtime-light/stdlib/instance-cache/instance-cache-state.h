// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <span>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

struct InstanceCacheInstanceState final : private vk::not_copyable {
  // per-request cache: key -> shared memory region published under that key
  // (layout: class_name_hash | class_instance shell | inner data, see f$instance_cache_store);
  // spans point to platform-owned memory that stays valid for the whole request lifetime
  kphp::stl::unordered_map<string, std::span<const std::byte>, kphp::memory::script_allocator,
                           decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); })>
      request_cache;

  InstanceCacheInstanceState() noexcept = default;
  static InstanceCacheInstanceState& get() noexcept;
};

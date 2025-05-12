// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-light/stdlib/instance-cache/request-cache.h"

struct InstanceCacheInstanceState final : private vk::not_copyable {
  InstanceCacheInstanceState() noexcept = default;

  kphp::instance_cache::RequestCache request_cache;
  static InstanceCacheInstanceState& get() noexcept;
};

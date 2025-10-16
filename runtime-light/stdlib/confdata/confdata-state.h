// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

class ConfdataInstanceState final : private vk::not_copyable {
  using hasher_type = decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); });

  kphp::stl::unordered_map<string, mixed, kphp::memory::script_allocator, hasher_type> m_key_cache;
  kphp::stl::unordered_map<string, array<mixed>, kphp::memory::script_allocator, hasher_type> m_wildcard_cache;

public:
  ConfdataInstanceState() noexcept = default;

  auto& key_cache() noexcept {
    return m_key_cache;
  }

  auto& wildcard_cache() noexcept {
    return m_wildcard_cache;
  }

  static ConfdataInstanceState& get() noexcept;
};

#pragma once

#include "runtime-common/core/runtime-core.h"

namespace instance_cache {

// Assume that components are not long-living,
// so there are no checks for TTL (as in KPHP runtime)
struct RequestCache final : private vk::not_copyable {
  array<mixed> storage;

  template<typename ClassInstanceType>
  void store(const string& key, const ClassInstanceType& instance) noexcept {
    storage[key] = f$to_mixed(instance);
  }

  template<typename ClassInstanceType>
  ClassInstanceType fetch(const string& key) noexcept {
    auto value = storage.get_value(key);
    return from_mixed<ClassInstanceType>(value, string());
  }

  void del(const string& key) noexcept {
    storage.unset(key);
  }
};
} // namespace instance_cache

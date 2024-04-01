// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <map>

#include "common/wrappers/likely.h"

#include "runtime/php_assert.h"

namespace memory_resource {

template<class T, class MemoryResource>
class resource_allocator {
public:
  using value_type = T;

  template<class, class>
  friend class resource_allocator;

  explicit resource_allocator(MemoryResource &memory_resource) noexcept
    : memory_resource_(memory_resource) {}

  template<class U>
  explicit resource_allocator(const resource_allocator<U, MemoryResource> &other) noexcept
    : memory_resource_(other.memory_resource_) {}

  value_type *allocate(size_t size, void const * = nullptr) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    php_assert(size == 1);
    auto result = static_cast<value_type *>(memory_resource_.allocate(sizeof(value_type)));
    if (unlikely(!result)) {
      php_critical_error("not enough memory to continue");
    }
    return result;
  }

  void deallocate(value_type *mem, size_t size) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    php_assert(size == 1);
    memory_resource_.deallocate(mem, sizeof(value_type));
  }

  static constexpr size_t max_value_type_size() {
    return 128u;
  }

  friend inline bool operator==(const resource_allocator &lhs, const resource_allocator &rhs) noexcept {
    return &lhs.memory_resource_ == &rhs.memory_resource_;
  }

  friend inline bool operator!=(const resource_allocator &lhs, const resource_allocator &rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  MemoryResource &memory_resource_;
};

namespace stl {
template<class Key, class Value, class Resource, class Cmp = std::less<Key>>
using map = std::map<Key, Value, Cmp, resource_allocator<std::pair<const Key, Value>, Resource>>;

template<class Key, class Value, class Resource, class Cmp = std::less<Key>>
using multimap = std::multimap<Key, Value, Cmp, resource_allocator<std::pair<const Key, Value>, Resource>>;
} // namespace stl

} // namespace memory_resource

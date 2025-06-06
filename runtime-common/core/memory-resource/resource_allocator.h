// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/wrappers/likely.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {

template<class T, class MemoryResource>
class resource_allocator {
public:
  using value_type = T;

  template<class, class>
  friend class resource_allocator;

  explicit resource_allocator(MemoryResource& memory_resource) noexcept
      : memory_resource_(memory_resource) {}

  template<class U>
  explicit resource_allocator(const resource_allocator<U, MemoryResource>& other) noexcept
      : memory_resource_(other.memory_resource_) {}

  value_type* allocate(size_t size, [[maybe_unused]] void const* ptr = nullptr) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    auto result = static_cast<value_type*>(memory_resource_.allocate(sizeof(value_type) * size));
    if (unlikely(!result)) {
      runtime_critical_error("not enough memory to continue");
    }
    return result;
  }

  void deallocate(value_type* mem, size_t size) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    memory_resource_.deallocate(mem, sizeof(value_type) * size);
  }

  static constexpr size_t max_value_type_size() {
    return 128U;
  }

  friend inline bool operator==(const resource_allocator& lhs, const resource_allocator& rhs) noexcept {
    return &lhs.memory_resource_ == &rhs.memory_resource_;
  }

  friend inline bool operator!=(const resource_allocator& lhs, const resource_allocator& rhs) noexcept {
    return !(lhs == rhs);
  }

private:
  MemoryResource& memory_resource_;
};

namespace stl {
template<class Key, class T, class Resource, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, resource_allocator<std::pair<const Key, T>, Resource>>;

template<class T, class Resource, class Hash = std::hash<T>, class KeyEqual = std::equal_to<T>>
using unordered_set = std::unordered_set<T, Hash, KeyEqual, resource_allocator<T, Resource>>;

template<class Key, class Value, class Resource, class Cmp = std::less<Key>>
using map = std::map<Key, Value, Cmp, resource_allocator<std::pair<const Key, Value>, Resource>>;

template<class Key, class Value, class Resource, class Cmp = std::less<Key>>
using multimap = std::multimap<Key, Value, Cmp, resource_allocator<std::pair<const Key, Value>, Resource>>;

template<class T, class Resource>
using deque = std::deque<T, resource_allocator<T, Resource>>;

template<class T, class Resource>
using queue = std::queue<T, std::deque<T, resource_allocator<T, Resource>>>;

template<class T, class Resource>
using vector = std::vector<T, resource_allocator<T, Resource>>;

template<class T, class Resource>
using list = std::list<T, resource_allocator<T, Resource>>;

template<class Resource>
using string = std::basic_string<char, std::char_traits<char>, resource_allocator<char, Resource>>;
} // namespace stl

} // namespace memory_resource

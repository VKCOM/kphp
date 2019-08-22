#pragma once

#include "common/wrappers/likely.h"

#include "runtime/php_assert.h"

namespace memory_resource {

template<class T, class MemoryResource>
class resource_allocator {
public:
  using value_type = T;

  template<class, class>
  friend
  class resource_allocator;

  explicit resource_allocator(MemoryResource &memory_resource) noexcept:
    memory_resource_(memory_resource) {
  }

  template<class U>
  explicit resource_allocator(const resource_allocator<U, MemoryResource> &other) noexcept:
    memory_resource_(other.memory_resource_) {
  }

  value_type *allocate(size_type size, void const * = nullptr) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    php_assert(size == 1);
    auto result = static_cast<value_type *>(memory_resource_.allocate(sizeof(value_type)));
    if (unlikely(!result)) {
      php_critical_error("not enough memory to continue");
    }
    return result;
  }

  void deallocate(value_type *mem, size_type size) {
    static_assert(sizeof(value_type) <= max_value_type_size(), "memory limit");
    php_assert(size == 1);
    memory_resource_.deallocate(mem, sizeof(value_type));
  }

  static constexpr size_type max_value_type_size() {
    return 128u;
  }

private:
  MemoryResource &memory_resource_;
};

} // namespace memory_resource

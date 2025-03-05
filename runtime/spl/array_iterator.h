// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

// C$ArrayIterator implements SPL ArrayIterator class.
struct C$ArrayIterator final : public refcountable_php_classes<C$ArrayIterator>, private DummyVisitorMethods {
  // we store an array to keep a living reference to it while iterator is valid;
  // also we may implement rewind() method later if we feel like it
  array<mixed> arr;
  array<mixed>::const_iterator it;
  array<mixed>::const_iterator end;

  const char *get_class() const noexcept {
    return "ArrayIterator";
  }

  int32_t get_hash() const noexcept {
    std::string_view name_view{C$ArrayIterator::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  using DummyVisitorMethods::accept;
};

void array_iterator_reset(class_instance<C$ArrayIterator> const &v$this, const array<mixed> &arr) noexcept;

class_instance<C$ArrayIterator> f$ArrayIterator$$__construct(class_instance<C$ArrayIterator> const &v$this, const array<mixed> &arr) noexcept;

inline bool f$ArrayIterator$$valid(class_instance<C$ArrayIterator> const &v$this) noexcept {
  return v$this->it != v$this->end;
}

inline int64_t f$ArrayIterator$$count(class_instance<C$ArrayIterator> const &v$this) noexcept {
  return v$this->arr.count();
}

inline mixed f$ArrayIterator$$current(class_instance<C$ArrayIterator> const &v$this) noexcept {
  return v$this->it != v$this->end ? v$this->it.get_value() : Optional<bool>{};
}

inline mixed f$ArrayIterator$$key(class_instance<C$ArrayIterator> const &v$this) noexcept {
  return v$this->it != v$this->end ? v$this->it.get_key() : Optional<bool>{};
}

inline void f$ArrayIterator$$next(class_instance<C$ArrayIterator> const &v$this) noexcept {
  if (v$this->it != v$this->end) {
    ++v$this->it;
  }
}

inline class_instance<C$ArrayIterator> f$reset_array_iterator(const class_instance<C$ArrayIterator> &iter, const array<mixed> &arr) {
  array_iterator_reset(iter, arr);
  return iter;
}

// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

#include "runtime/dummy-visitor-methods.h"
#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"

// C$ArrayIterator implements SPL ArrayIterator class.
struct C$ArrayIterator : public refcountable_php_classes<C$ArrayIterator>, private DummyVisitorMethods {
  // we store an array to keep a living reference to it while iterator is valid;
  // also we may implement rewind() method later if we feel like it
  array<mixed> arr;
  array<mixed>::const_iterator it;
  array<mixed>::const_iterator end;

  const char *get_class() const noexcept {
    return "ArrayIterator";
  }

  int32_t get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(get_class())));
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

// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/spl/array_iterator.h"

void array_iterator_reset(const class_instance<C$ArrayIterator>& iter, const array<mixed>& arr) noexcept {
  iter->arr = arr;
  iter->it = const_begin(arr);
  iter->end = const_end(arr);
}

class_instance<C$ArrayIterator> f$ArrayIterator$$__construct(class_instance<C$ArrayIterator> const& v$this, const array<mixed>& arr) noexcept {
  array_iterator_reset(v$this, arr);
  return v$this;
}

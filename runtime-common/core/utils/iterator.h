// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <functional>
#include <iterator>

#include "runtime-common/core/runtime-core.h"

namespace kphp {

struct string_back_insert_iterator {
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = ptrdiff_t;
  using pointer = void;
  using reference = void;

  std::reference_wrapper<string> ref;

  string_back_insert_iterator& operator=(char value) noexcept {
    ref.get().push_back(value);
    return *this;
  }

  string_back_insert_iterator& operator*() noexcept {
    return *this;
  }

  string_back_insert_iterator& operator++() noexcept {
    return *this;
  }

  string_back_insert_iterator operator++(int) noexcept { // NOLINT
    return *this;
  }
};

} // namespace kphp

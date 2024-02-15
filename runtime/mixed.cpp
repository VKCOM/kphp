// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_core.h"

void mixed::destroy() noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().~string();
      break;
    case type::ARRAY:
      as_array().~array<mixed>();
      break;
    default: {
    }
  }
}

void mixed::clear() noexcept {
  destroy();
  type_ = type::NUL;
}

// Don't move this destructor to the headers, it spoils addr2line traces
mixed::~mixed() noexcept {
  clear();
}

static_assert(sizeof(mixed) == SIZEOF_MIXED, "sizeof(mixed) at runtime doesn't match compile-time");

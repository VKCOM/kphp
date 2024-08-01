// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-core/runtime-core.h"

void mixed::destroy() noexcept {
  switch (get_type()) {
    case type::STRING:
      as_string().~string();
      break;
    case type::ARRAY:
      as_array().~array<mixed>();
      break;
    case type::OBJECT: {
      auto ptr = *reinterpret_cast<vk::intrusive_ptr<may_be_mixed_base>*>(&storage_);
      ptr.~intrusive_ptr<may_be_mixed_base>();
      storage_ = 0;
      break;
    }
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
static_assert(sizeof(Unknown) == SIZEOF_UNKNOWN, "sizeof(Unknown) at runtime doesn't match compile-time");

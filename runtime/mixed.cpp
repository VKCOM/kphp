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
    case type::OBJECT: {
      fputs("[DESTROYING OBJ IN MIXED]\n", stderr);
      auto *outter_ptr = reinterpret_cast<const class_instance<may_be_mixed_base> *>(&storage_);
      auto *refcnt_ptr = outter_ptr->get();
      fprintf(stderr, "outter_ptr: %p\n", outter_ptr);
      fprintf(stderr, "refcnt_ptr: %p\n", refcnt_ptr);
//      fprintf(stderr, "deref outter: %p\n", )
      if (refcnt_ptr) {
        fputs("[... DO DECREF]\n---release\n", stderr);
        refcnt_ptr->release(); // Call release for class that inside intrusive pointer inside class_instance
      } else {
        fputs("[...NOT DECREF!]\n", stderr);  
      }
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

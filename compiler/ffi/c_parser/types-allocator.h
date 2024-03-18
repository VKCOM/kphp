// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/ffi/ffi_types.h"

#include <vector>

namespace ffi {

// TypesAllocator is used to simplify the memory management in the C parser.
//
// We allocate types with new_type and record allocated pointer in the vector.
// We never call delete/free from the outside, instead, we mark all reachable (used)
// types with a special flag. All types that were allocated but are not marked
// with that flag are removed.
//
// This allows us to forget about Bison %destructor handlers as well as
// keep our grammar (c.y) file clean, without chaotic delete statements.
//
// Since FFI types are quite rare in comparison with normal PHP types,
// we can allow ourselves such convenient allocator.
//
// Note: it's not thread-safe.
// TypesAllocator should not be shared between several C parsers.
class TypesAllocator {
public:
  template<class... Args>
  FFIType *new_type(Args &&...args) {
    auto *type = new FFIType{std::forward<Args>(args)...};
    allocated_.push_back(type);
    return type;
  }

  uint64_t num_allocated() const noexcept {
    return allocated_.size();
  }

  void mark_used(FFIType *type, bool recursive = true) const {
    type->flags = static_cast<FFIType::Flag>(type->flags | FFIType::Flag::Used);
    if (recursive) {
      for (FFIType *member : type->members) {
        mark_used(member, recursive);
      }
    }
  }

  uint64_t collect_garbage() const {
    uint64_t num_deleted = 0;
    for (FFIType *type : allocated_) {
      if ((type->flags & FFIType::Flag::Used) == 0) {
        delete type;
        num_deleted++;
      }
    }
    return num_deleted;
  }

private:
  std::vector<FFIType *> allocated_;
};

} // namespace ffi

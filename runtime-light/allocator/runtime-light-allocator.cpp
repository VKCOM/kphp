// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/allocator/allocator-state.h"

RuntimeAllocator& RuntimeAllocator::get() noexcept {
  return AllocatorState::get_mutable().allocator;
}

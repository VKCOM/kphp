// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"

void ArrayAccessContext::free() noexcept {
  array_access_slot_[0].clear();
  array_access_slot_[1].clear();
}

mixed &ArrayAccessContext::get_tmp_slot() noexcept {
  slot_idx = (slot_idx + 1) % 2;
  return array_access_slot_[slot_idx];
}

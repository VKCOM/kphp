// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "common/mixin/not_copyable.h"

struct ArrayAccessContext final {
  mixed array_access_slot_[2]{};
  size_t slot_idx{};

  void free() noexcept;
  mixed &get_tmp_slot() noexcept;
};

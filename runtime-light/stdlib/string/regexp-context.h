// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct RegexpImageState final : private vk::not_copyable {
  string_buffer regexp_sb;

  static const RegexpImageState &get() noexcept;
  static RegexpImageState &get_mutable() noexcept;
};

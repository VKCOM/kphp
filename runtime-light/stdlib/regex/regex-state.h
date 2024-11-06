// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"

struct RegexInstanceState final : private vk::not_copyable {
  int64_t preg_replace_count_dummy{};

  static RegexInstanceState &get() noexcept;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

struct StringComponentContext {
  int64_t str_replace_count_dummy{};
  double default_similar_text_percent_stub{};

  static StringComponentContext &get() noexcept;
};

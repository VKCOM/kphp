// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

struct KphpCoreContext {
  static KphpCoreContext& current() noexcept;
  void init();
  void free();

  int show_migration_php8_warning = 0;
  uint64_t empty_alloc_counter = 0;
  string_buffer_lib_context sb_lib_context;
};

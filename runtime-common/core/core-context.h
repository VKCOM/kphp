// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"

#ifndef INCLUDED_FROM_KPHP_CORE
#error "this file must be included only from runtime-core.h"
#endif

struct RuntimeContext final : vk::not_copyable {
  int32_t show_migration_php8_warning{};
  int32_t php_disable_warnings{};
  uint32_t empty_obj_count{};

  string_buffer_lib_context sb_lib_context{};
  string_buffer static_SB{};
  string_buffer static_SB_spare{};

  void init() noexcept;
  void free() noexcept;

  static RuntimeContext &get() noexcept;
};

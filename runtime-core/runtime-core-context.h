// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif


struct KphpCoreContext {
  /**
   * KphpCoreContext is used in
   * @see init_php_scripts_once_in_master for runtime or
   * @see vk_k2_create_image_state for runtime light
   *
   * before the init() function is called, so its default parameters should be as follows
   **/
  static KphpCoreContext& current() noexcept;

  void init();
  void free();

  int show_migration_php8_warning = 0;
  int php_disable_warnings = 0;
  uint32_t empty_obj_count = 0;
  string_buffer_lib_context sb_lib_context;
};

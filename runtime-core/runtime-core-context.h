// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

struct RuntimeAllocator {
  static RuntimeAllocator& current() noexcept;

  RuntimeAllocator() = default;
  RuntimeAllocator(size_t script_mem_size, size_t oom_handling_mem_size);

  void init(void * buffer, size_t script_mem_size, size_t oom_handling_mem_size);
  void free();

  void * alloc_script_memory(size_t size) noexcept;
  void * alloc0_script_memory(size_t size) noexcept;
  void * realloc_script_memory(void *mem, size_t new_size, size_t old_size) noexcept;
  void free_script_memory(void *mem, size_t size) noexcept;

  void * alloc_global_memory(size_t size) noexcept;
  void * alloc0_global_memory(size_t size) noexcept;
  void * realloc_global_memory(void *mem, size_t new_size, size_t old_size) noexcept;
  void free_global_memory(void *mem, size_t size) noexcept;

  memory_resource::unsynchronized_pool_resource memory_resource;
};

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
  uint32_t empty_obj_count = 0;
  string_buffer_lib_context sb_lib_context;
};

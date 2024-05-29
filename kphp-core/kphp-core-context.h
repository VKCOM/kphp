#pragma once

#include <cstddef>

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

struct CoreAllocator {
  void * alloc_script_memory(size_t size);
  void * alloc0_script_memory(size_t size);
  void * realloc_script_memory(void *mem, size_t new_size, size_t old_size);
  void free_script_memory(void *mem, size_t size);

  void * alloc_global_memory(size_t size);
  void * realloc_global_memory(void *mem, size_t new_size, size_t old_size);
  void free_global_memory(void *mem, size_t size);
};

struct KphpCoreContext {
  CoreAllocator allocator;

  static KphpCoreContext& current() noexcept;
  void init();
  void free();

  int show_migration_php8_warning = 0;
  string_buffer_lib_context sb_lib_context;
};

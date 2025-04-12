// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/context/runtime-context.h"

#include "runtime-common/core/runtime-core.h"
#include "server/php-engine-vars.h"

RuntimeContext kphp_runtime_context;
RuntimeAllocator runtime_allocator;

RuntimeContext& RuntimeContext::get() noexcept {
  return kphp_runtime_context;
}

void RuntimeContext::init() noexcept {
  if (static_buffer_length_limit < 0) {
    init_string_buffer_lib(266175, (1 << 24));
  } else {
    init_string_buffer_lib(266175, static_buffer_length_limit);
  }
}

void RuntimeContext::free() noexcept {
  free_migration_php8();
}

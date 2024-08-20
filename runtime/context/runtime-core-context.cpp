// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/smart_ptrs/singleton.h"
#include "runtime/context/runtime-context.h"
#include "server/php-engine-vars.h"
#include "runtime/allocator.h"

KphpCoreContext &KphpCoreContext::current() noexcept {
  return kphp_runtime_context;
}

void KphpCoreContext::init() {
  if (static_buffer_length_limit < 0) {
    init_string_buffer_lib(266175, (1 << 24));
  } else {
    init_string_buffer_lib(266175, static_buffer_length_limit);
  }
}

void KphpCoreContext::free() {
  free_migration_php8();
}

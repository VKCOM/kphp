// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "php-script-globals.h"

#include "compiler/inferring/key.h"

#include "runtime-light/component/component.h"

PhpScriptMutableGlobals &PhpScriptMutableGlobals::current() {
  return get_component_context()->php_script_mutable_globals_singleton;
}

void PhpScriptMutableGlobals::once_alloc_linear_mem(unsigned int n_bytes) {
  php_assert(g_linear_mem == nullptr);
  g_linear_mem = static_cast<char *>(get_platform_context()->allocator.alloc(n_bytes));
  memset(g_linear_mem, 0, n_bytes);
}

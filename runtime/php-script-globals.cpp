// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/php-script-globals.h"

static PhpScriptMutableGlobals php_script_mutable_globals_singleton;

PhpScriptMutableGlobals &PhpScriptMutableGlobals::current() {
  return php_script_mutable_globals_singleton;
}

void PhpScriptMutableGlobals::once_alloc_linear_mem(unsigned int n_bytes) {
  php_assert(g_linear_mem == nullptr);
  g_linear_mem = new char[n_bytes];
  memset(g_linear_mem, 0, n_bytes);
}

void PhpScriptMutableGlobals::once_alloc_linear_mem(const char *lib_name, unsigned int n_bytes) {
  int64_t key_lib_name = string_hash(lib_name, strlen(lib_name));
  php_assert(libs_linear_mem.find(key_lib_name) == libs_linear_mem.end());
  libs_linear_mem[key_lib_name] = new char[n_bytes];
  memset(libs_linear_mem[key_lib_name], 0, n_bytes);
}

char *PhpScriptMutableGlobals::mem_for_lib(const char *lib_name) const {
  int64_t key_lib_name = string_hash(lib_name, strlen(lib_name));
  auto found = libs_linear_mem.find(key_lib_name);
  php_assert(found != libs_linear_mem.end());
  return found->second;
}

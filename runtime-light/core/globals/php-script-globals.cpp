// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "php-script-globals.h"

#include "runtime-light/state/instance-state.h"
#include "runtime-light/utils/logs.h"

PhpScriptMutableGlobals& PhpScriptMutableGlobals::current() noexcept {
  return InstanceState::get().php_script_mutable_globals_singleton;
}

void PhpScriptMutableGlobals::once_alloc_linear_mem(unsigned int n_bytes) {
  kphp::log::assertion(g_linear_mem == nullptr);
  g_linear_mem = static_cast<char*>(RuntimeAllocator::get().alloc0_global_memory(n_bytes));
}

void PhpScriptMutableGlobals::once_alloc_linear_mem(const char* lib_name, unsigned int n_bytes) {
  int64_t key_lib_name{string_hash(lib_name, strlen(lib_name))};
  kphp::log::assertion(libs_linear_mem.find(key_lib_name) == libs_linear_mem.end());
  libs_linear_mem[key_lib_name] = static_cast<char*>(RuntimeAllocator::get().alloc0_global_memory(n_bytes));
}

char* PhpScriptMutableGlobals::get_linear_mem(const char* lib_name) const {
  int64_t key_lib_name{string_hash(lib_name, strlen(lib_name))};
  auto found{libs_linear_mem.find(key_lib_name)};
  kphp::log::assertion(found != libs_linear_mem.end());
  return found->second;
}

char* PhpScriptMutableGlobals::mem_for_lib(const char* lib_name) const {
  int64_t key_lib_name{string_hash(lib_name, strlen(lib_name))};
  auto found{libs_linear_mem.find(key_lib_name)};
  kphp::log::assertion(found != libs_linear_mem.end());
  return found->second;
}

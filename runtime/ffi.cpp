// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/ffi.h"

#include <dlfcn.h>

FFIEnv ffi_env_instance;

FFIEnv::FFIEnv(int num_libs, int num_symbols)
    : num_libs{num_libs},
      libs{new SharedLib[num_libs]},
      num_symbols{num_symbols},
      symbols{new Symbol[num_symbols]} {}

bool FFIEnv::is_shared_lib_opened(int id) {
  return libs[id].handle != nullptr;
}

bool FFIEnv::is_scope_loaded(int sym_offset) {
  return symbols[sym_offset].ptr != nullptr;
}

// open_shared_libs performs an actual shared lib open operation;
// only once per shared_lib_id, several scopes can use this library
void FFIEnv::open_shared_lib(int id) {
  void* handle = funcs.dlopen(libs[id].path, RTLD_LAZY);
  if (!handle) {
    php_critical_error("can't open %s library", libs[id].path);
  }
  libs[id].handle = handle;
}

void FFIEnv::load_scope_symbols(int id, int sym_offset, int num_lib_symbols) {
  // bind all symbols; this should happen exactly once per every scope,
  // but some shared libraries may be referenced by multiple FFI scopes;
  // in this case, we'll do some redundant dlopen to provide every
  // FFI scope its own view on the shared library
  for (int i = sym_offset; i < sym_offset + num_lib_symbols; i++) {
    load_symbol(id, i);
  }
}

void FFIEnv::load_symbol(int lib_id, int dst_sym_id) {
  const char* symbol_name = symbols[dst_sym_id].name;
  void* symbol = funcs.dlsym(libs[lib_id].handle, symbol_name);
  if (!symbol) {
    php_warning("%s library doesn't export %s symbol", libs[lib_id].path, symbol_name);
    return;
  }
  symbols[dst_sym_id].ptr = symbol;
}

class_instance<C$FFI$Scope> f$FFI$$load(const string& filename __attribute__((unused))) {
  return class_instance<C$FFI$Scope>().empty_alloc();
}

class_instance<C$FFI$Scope> f$FFI$$scope(const string& scope_name __attribute__((unused))) {
  return class_instance<C$FFI$Scope>().empty_alloc();
}

class_instance<C$FFI$Scope> ffi_load_scope_symbols(class_instance<C$FFI$Scope> instance, int shared_lib_id, int sym_offset, int num_symbols) {
  // fast path happens if shared lib is already loaded as well as
  // scope-related symbols; in the worst case we do 1 dlopen
  // plus $num_symbols calls to dlsym to load symbols; in the rare case
  // of several scopes using the same shared lib we only do
  // $num_symbols calls to dlsym without doing dlopen again
  //
  // if we run ffi_load_scope_symbols() in the loop, all
  // calls except the first one will go through a fast path

  if (!ffi_env_instance.is_shared_lib_opened(shared_lib_id)) {
    // a sanity check: library symbols should not be initialized yet
    php_assert(ffi_env_instance.symbols[sym_offset].ptr == nullptr);
    ffi_env_instance.open_shared_lib(shared_lib_id);
  }

  if (!ffi_env_instance.is_scope_loaded(sym_offset)) {
    ffi_env_instance.load_scope_symbols(shared_lib_id, sym_offset, num_symbols);
  }

  return instance;
}

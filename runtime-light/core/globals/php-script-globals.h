// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/memory-resource/resource_allocator.h"

struct PhpScriptBuiltInSuperGlobals {
  // variables below are PHP language superglobals
  mixed v$_SERVER;
  mixed v$_GET;
  mixed v$_POST;
  mixed v$_ENV;
  mixed v$_FILES;
  mixed v$_COOKIE;
  mixed v$_REQUEST;

  // variables below are not superglobals of the PHP language, but since they are set by runtime,
  // the compiler is also aware about them
  mixed v$argc;
  mixed v$argv;
  string v$d$PHP_SAPI;  // define('PHP_SAPI')
};

class PhpScriptMutableGlobals {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  char *g_linear_mem{nullptr};
  unordered_map<int64_t, char *> libs_linear_mem;
  PhpScriptBuiltInSuperGlobals superglobals;

public:
  PhpScriptMutableGlobals(memory_resource::unsynchronized_pool_resource & resource)
    : libs_linear_mem(unordered_map<int64_t, char *>::allocator_type{resource}) {}

  static PhpScriptMutableGlobals &current() noexcept;

  void once_alloc_linear_mem(unsigned int n_bytes);
  void once_alloc_linear_mem(const char *lib_name, unsigned int n_bytes);

  char *get_linear_mem() const { return g_linear_mem; }
  char *get_linear_mem(const char *lib_name) const;

  char *mem() const { return g_linear_mem; }
  char *mem_for_lib(const char *lib_name) const;

  PhpScriptBuiltInSuperGlobals &get_superglobals() { return superglobals; }
  const PhpScriptBuiltInSuperGlobals &get_superglobals() const { return superglobals; }
};

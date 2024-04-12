// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/allocator/memory_resource/resource_allocator.h"

#include "runtime-light/core/kphp_core.h"

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
  PhpScriptBuiltInSuperGlobals superglobals;
  // todo:k2 implement mem for libs globals

public:
  static PhpScriptMutableGlobals &current();

  void once_alloc_linear_mem(unsigned int n_bytes);

  char *get_linear_mem() const { return g_linear_mem; }

  PhpScriptBuiltInSuperGlobals &get_superglobals() { return superglobals; }
  const PhpScriptBuiltInSuperGlobals &get_superglobals() const { return superglobals; }
};

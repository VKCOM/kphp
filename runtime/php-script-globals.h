// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <unordered_map>

#include "runtime/kphp_core.h"

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
  char *g_linear_mem{nullptr};
  std::unordered_map<int64_t, char *> libs_linear_mem;
  PhpScriptBuiltInSuperGlobals superglobals;

public:
  static PhpScriptMutableGlobals &current();

  void once_alloc_linear_mem(unsigned int n_bytes);
  void once_alloc_linear_mem(const char *lib_name, unsigned int n_bytes);

  char *get_linear_mem() const { return g_linear_mem; }
  char *get_linear_mem(const char *lib_name) const;

  PhpScriptBuiltInSuperGlobals &get_superglobals() { return superglobals; }
  const PhpScriptBuiltInSuperGlobals &get_superglobals() const { return superglobals; }
};

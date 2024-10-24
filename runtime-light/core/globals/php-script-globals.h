// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"

namespace PhpServerSuperGlobalIndices {

inline constexpr auto *ARGC = "argc";
inline constexpr auto *ARGV = "argv";

inline constexpr auto *PHP_SELF = "PHP_SELF";
inline constexpr auto *SCRIPT_URL = "SCRIPT_URL";
inline constexpr auto *SCRIPT_URI = "SCRIPT_URI";
inline constexpr auto *SCRIPT_NAME = "SCRIPT_NAME";

inline constexpr auto *REQUEST_URI = "REQUEST_URI";
inline constexpr auto *REQUEST_TIME = "REQUEST_TIME";
inline constexpr auto *REQUEST_METHOD = "REQUEST_METHOD";
inline constexpr auto *REQUEST_TIME_FLOAT = "REQUEST_TIME_FLOAT";

inline constexpr auto *JOB_ID = "JOB_ID";

inline constexpr auto *SERVER_NAME = "SERVER_NAME";
inline constexpr auto *SERVER_ADDR = "SERVER_ADDR";
inline constexpr auto *SERVER_PORT = "SERVER_PORT";
inline constexpr auto *SERVER_PROTOCOL = "SERVER_PROTOCOL";
inline constexpr auto *SERVER_SOFTWARE = "SERVER_SOFTWARE";
inline constexpr auto *SERVER_SIGNATURE = "SERVER_SIGNATURE";

inline constexpr auto *REMOTE_ADDR = "REMOTE_ADDR";
inline constexpr auto *REMOTE_PORT = "REMOTE_PORT";

inline constexpr auto *QUERY_STRING = "QUERY_STRING";
inline constexpr auto *GATEWAY_INTERFACE = "GATEWAY_INTERFACE";

}; // namespace PhpServerSuperGlobalIndices

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
  string v$d$PHP_SAPI; // define('PHP_SAPI')
};

class PhpScriptMutableGlobals {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;
  char *g_linear_mem{nullptr};
  unordered_map<int64_t, char *> libs_linear_mem;
  PhpScriptBuiltInSuperGlobals superglobals;

public:
  explicit PhpScriptMutableGlobals(memory_resource::unsynchronized_pool_resource &resource)
    : libs_linear_mem(unordered_map<int64_t, char *>::allocator_type{resource}) {}

  static PhpScriptMutableGlobals &current() noexcept;

  void once_alloc_linear_mem(unsigned int n_bytes);
  void once_alloc_linear_mem(const char *lib_name, unsigned int n_bytes);

  char *get_linear_mem() const {
    return g_linear_mem;
  }
  char *get_linear_mem(const char *lib_name) const;

  char *mem() const {
    return g_linear_mem;
  }
  char *mem_for_lib(const char *lib_name) const;

  PhpScriptBuiltInSuperGlobals &get_superglobals() {
    return superglobals;
  }
  const PhpScriptBuiltInSuperGlobals &get_superglobals() const {
    return superglobals;
  }
};

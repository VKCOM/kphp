// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-common/core/std/containers.h"

namespace PhpServerSuperGlobalIndices {

inline constexpr std::string_view ARGC = "argc";
inline constexpr std::string_view ARGV = "argv";

inline constexpr std::string_view PHP_SELF = "PHP_SELF";
inline constexpr std::string_view SCRIPT_URL = "SCRIPT_URL";
inline constexpr std::string_view SCRIPT_URI = "SCRIPT_URI";
inline constexpr std::string_view SCRIPT_NAME = "SCRIPT_NAME";

inline constexpr std::string_view REQUEST_URI = "REQUEST_URI";
inline constexpr std::string_view REQUEST_TIME = "REQUEST_TIME";
inline constexpr std::string_view REQUEST_METHOD = "REQUEST_METHOD";
inline constexpr std::string_view REQUEST_TIME_FLOAT = "REQUEST_TIME_FLOAT";

inline constexpr std::string_view JOB_ID = "JOB_ID";

inline constexpr std::string_view SERVER_NAME = "SERVER_NAME";
inline constexpr std::string_view SERVER_ADDR = "SERVER_ADDR";
inline constexpr std::string_view SERVER_PORT = "SERVER_PORT";
inline constexpr std::string_view SERVER_PROTOCOL = "SERVER_PROTOCOL";
inline constexpr std::string_view SERVER_SOFTWARE = "SERVER_SOFTWARE";
inline constexpr std::string_view SERVER_SIGNATURE = "SERVER_SIGNATURE";

inline constexpr std::string_view REMOTE_ADDR = "REMOTE_ADDR";
inline constexpr std::string_view REMOTE_PORT = "REMOTE_PORT";

inline constexpr std::string_view QUERY_STRING = "QUERY_STRING";
inline constexpr std::string_view GATEWAY_INTERFACE = "GATEWAY_INTERFACE";

inline constexpr std::string_view CONTENT_TYPE = "CONTENT_TYPE";

inline constexpr std::string_view AUTH_TYPE = "AUTH_TYPE";
inline constexpr std::string_view PHP_AUTH_USER = "PHP_AUTH_USER";
inline constexpr std::string_view PHP_AUTH_PW = "PHP_AUTH_PW";

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
  using unordered_map = kphp::stl::unordered_map<Key, Value, kphp::memory::script_allocator>;

  char *g_linear_mem{nullptr};
  unordered_map<int64_t, char *> libs_linear_mem;
  PhpScriptBuiltInSuperGlobals superglobals;

public:
  PhpScriptMutableGlobals() noexcept = default;

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

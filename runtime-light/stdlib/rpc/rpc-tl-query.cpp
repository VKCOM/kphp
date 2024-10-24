//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-query.h"

#include <array>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"

void CurrentTlQuery::reset() noexcept {
  current_tl_function_name = string{};
}

void CurrentTlQuery::set_current_tl_function(const string &tl_function_name) noexcept {
  // It can be not empty in the following case:
  // 1. Timeout is raised in the middle of serialization (when current TL function is still not reset).
  // 2. Then shutdown functions called from timeout.
  // 3. They use RPC which finally call set_current_tl_function.
  // It will be rewritten by another tl_function_name and work fine
  current_tl_function_name = tl_function_name;
}

void CurrentTlQuery::set_current_tl_function(const class_instance<RpcTlQuery> &current_query) noexcept {
  current_tl_function_name = current_query.get()->tl_function_name;
}

void CurrentTlQuery::raise_fetching_error(const char *format, ...) const noexcept {
  php_assert(!current_tl_function_name.empty());

  if (/* !CurException.is_null() */ false) {
    return;
  }

  constexpr size_t BUFF_SZ = 1024;
  std::array<char, BUFF_SZ> buff{};

  va_list args;
  va_start(args, format);
  int32_t sz = vsnprintf(buff.data(), BUFF_SZ, format, args);
  php_assert(sz > 0);
  va_end(args);

  string msg = string(buff.data(), static_cast<size_t>(sz));
  php_warning("Fetching error:\n%s\nIn %s deserializing TL object", msg.c_str(), current_tl_function_name.c_str());
  msg.append(string(" in result of ")).append(current_tl_function_name);
  //    THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
}

void CurrentTlQuery::raise_storing_error(const char *format, ...) const noexcept {
  if (/*!CurException.is_null()*/ false) {
    return;
  }

  constexpr size_t BUFF_SZ = 1024;
  std::array<char, BUFF_SZ> buff{};

  va_list args;
  va_start(args, format);
  int32_t sz = vsnprintf(buff.data(), BUFF_SZ, format, args);
  php_assert(sz > 0);
  va_end(args);

  string msg = string(buff.data(), static_cast<size_t>(sz));
  php_warning("Storing error:\n%s\nIn %s serializing TL object", msg.c_str(),
              current_tl_function_name.empty() ? "_unknown_" : current_tl_function_name.c_str());
  //  THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
}

void CurrentTlQuery::set_last_stored_tl_function_magic(uint32_t tl_magic) noexcept {
  last_stored_tl_function_magic = tl_magic;
}

uint32_t CurrentTlQuery::get_last_stored_tl_function_magic() const noexcept {
  return last_stored_tl_function_magic;
}

const string &CurrentTlQuery::get_current_tl_function_name() const noexcept {
  return current_tl_function_name;
}

CurrentTlQuery &CurrentTlQuery::get() noexcept {
  return RpcComponentContext::get().current_query;
}

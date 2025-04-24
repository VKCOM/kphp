//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-tl-func-base.h"
#include "runtime-light/stdlib/rpc/rpc-tl-request.h"

struct RpcTlQuery : refcountable_php_classes<RpcTlQuery> {
  string tl_function_name;
  std::unique_ptr<RpcRequestResult> result_fetcher;
};

struct CurrentTlQuery {
  void reset() noexcept {
    current_tl_function_name = string{};
  }

  void set_current_tl_function(const string& tl_function_name) noexcept {
    // It can be not empty in the following case:
    // 1. Timeout is raised in the middle of serialization (when current TL function is still not reset).
    // 2. Then shutdown functions called from timeout.
    // 3. They use RPC which finally call set_current_tl_function.
    // It will be rewritten by another tl_function_name and work fine
    current_tl_function_name = tl_function_name;
  }

  void set_current_tl_function(const class_instance<RpcTlQuery>& current_query) noexcept {
    current_tl_function_name = current_query.get()->tl_function_name;
  }

  void raise_fetching_error(const char* format, ...) const noexcept __attribute__((format(printf, 2, 3)));
  void raise_storing_error(const char* format, ...) const noexcept __attribute__((format(printf, 2, 3)));

  // called from generated TL serializers (from autogen)
  void set_last_stored_tl_function_magic(uint32_t tl_magic) noexcept {
    last_stored_tl_function_magic = tl_magic;
  }

  uint32_t get_last_stored_tl_function_magic() const noexcept {
    return last_stored_tl_function_magic;
  }

  const string& get_current_tl_function_name() const noexcept {
    return current_tl_function_name;
  }

  static CurrentTlQuery& get() noexcept;

private:
  string current_tl_function_name;
  uint32_t last_stored_tl_function_magic{};
};

struct CurrentRpcServerQuery {
  void save(std::unique_ptr<tl_func_base> tl_func_base) noexcept {
    query = std::move(tl_func_base);
  }

  std::unique_ptr<tl_func_base> extract() noexcept {
    return std::move(query);
  }

  void reset() noexcept {
    query.reset();
  }

  static CurrentRpcServerQuery& get() noexcept;

private:
  std::unique_ptr<tl_func_base> query;
};

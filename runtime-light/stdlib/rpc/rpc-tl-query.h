//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>

#include "runtime-core/class-instance/refcountable-php-classes.h"
#include "runtime-core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-tl-request.h"

struct RpcTlQuery : refcountable_php_classes<RpcTlQuery> {
  string tl_function_name;
  std::unique_ptr<RpcRequestResult> result_fetcher;
};

struct CurrentTlQuery {
  void reset() noexcept;
  void set_current_tl_function(const string &tl_function_name) noexcept;
  void set_current_tl_function(const class_instance<RpcTlQuery> &current_query) noexcept;
  void raise_fetching_error(const char *format, ...) const noexcept __attribute__((format(printf, 2, 3)));
  void raise_storing_error(const char *format, ...) const noexcept __attribute__((format(printf, 2, 3)));

  // called from generated TL serializers (from autogen)
  void set_last_stored_tl_function_magic(uint32_t tl_magic) noexcept;
  uint32_t get_last_stored_tl_function_magic() const noexcept;
  const string &get_current_tl_function_name() const noexcept;

private:
  string current_tl_function_name;
  uint32_t last_stored_tl_function_magic{0};
};

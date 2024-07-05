//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "runtime-core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-tl-func-base.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"
#include "runtime-light/utils/php_assert.h"

using tl_undefined_php_type = std::nullptr_t;
using tl_storer_ptr = std::unique_ptr<tl_func_base> (*)(const mixed &);
using tl_fetch_wrapper_ptr = array<mixed> (*)(std::unique_ptr<tl_func_base>);

struct tl_exclamation_fetch_wrapper {
  std::unique_ptr<tl_func_base> fetcher;

  explicit tl_exclamation_fetch_wrapper(std::unique_ptr<tl_func_base> fetcher)
    : fetcher(std::move(fetcher)) {}

  tl_exclamation_fetch_wrapper() noexcept = default;
  tl_exclamation_fetch_wrapper(const tl_exclamation_fetch_wrapper &) = delete;
  tl_exclamation_fetch_wrapper(tl_exclamation_fetch_wrapper &&) noexcept = default;
  tl_exclamation_fetch_wrapper &operator=(const tl_exclamation_fetch_wrapper &) = delete;
  tl_exclamation_fetch_wrapper &operator=(tl_exclamation_fetch_wrapper &&) noexcept = delete;
  ~tl_exclamation_fetch_wrapper() = default;

  mixed fetch() const {
    return fetcher->fetch();
  }

  using PhpType = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  void typed_fetch_to(PhpType &out) const {
    php_assert(fetcher);
    out = fetcher->typed_fetch();
  }
};

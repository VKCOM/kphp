// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/tl/tl-core.h"

class RpcServerInstanceState final : vk::not_copyable {
public:
  int64_t query_id{};

  tl::TLBuffer buffer;
  CurrentTlQuery current_query{};
  bool fail_rpc_on_int32_overflow{};

  RpcServerInstanceState() noexcept = default;

  static RpcServerInstanceState& get() noexcept;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"

struct RpcServerInstanceState final : vk::not_copyable {
  tl::storer tl_storer;
  tl::fetcher tl_fetcher{tl_storer.view()};
  std::optional<kphp::component::stream> request_stream;

  bool fail_rpc_on_int32_overflow{};
  int64_t query_id{kphp::rpc::INVALID_QUERY_ID};
  CurrentRpcServerQuery current_server_query{};

  RpcServerInstanceState() noexcept = default;

  static RpcServerInstanceState& get() noexcept;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/rpc/rpc-constants.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"

struct RpcClientInstanceState final : private vk::not_copyable {
  template<typename Key, typename Value>
  using unordered_map = kphp::stl::unordered_map<Key, Value, kphp::memory::script_allocator>;

  CurrentTlQuery current_client_query{};
  int64_t current_query_id{kphp::rpc::VALID_QUERY_ID_RANGE_START};

  unordered_map<int64_t, int64_t> response_waiter_forks;
  unordered_map<int64_t, class_instance<RpcTlQuery>> response_fetcher_instances;
  unordered_map<int64_t, std::pair<kphp::rpc::response_extra_info_status, kphp::rpc::response_extra_info>> rpc_responses_extra_info;

  // An analogue of the response_waiter_forks mapping,
  // which stores mappings from fork_id to response_id for responses awaited in the rpc queue.
  kphp::stl::unordered_map<int64_t, int64_t, kphp::memory::script_allocator> awaiter_forks_to_response;

  RpcClientInstanceState() noexcept = default;

  static RpcClientInstanceState& get() noexcept;
};

// ================================================================================================

struct RpcImageState final : private vk::not_copyable {
  array<tl_storer_ptr> tl_storers_ht;
  tl_fetch_wrapper_ptr tl_fetch_wrapper{nullptr};

  static const RpcImageState& get() noexcept;
  static RpcImageState& get_mutable() noexcept;
};

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
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/tl/tl-core.h"

namespace kphp::rpc {

inline constexpr int64_t VALID_QUERY_ID_RANGE_START = 1;
inline constexpr int64_t INVALID_QUERY_ID = 0;
inline constexpr int64_t IGNORED_ANSWER_QUERY_ID = -1;

} // namespace kphp::rpc

struct RpcClientInstanceState final : private vk::not_copyable {
  template<typename Key, typename Value>
  using unordered_map = kphp::stl::unordered_map<Key, Value, kphp::memory::script_allocator>;

  tl::TLBuffer rpc_buffer;
  int64_t current_query_id{kphp::rpc::VALID_QUERY_ID_RANGE_START};
  CurrentTlQuery current_query{};
  bool fail_rpc_on_int32_overflow{};
  unordered_map<int64_t, int64_t> response_waiter_forks;
  unordered_map<int64_t, class_instance<RpcTlQuery>> response_fetcher_instances;
  unordered_map<int64_t, std::pair<kphp::rpc::response_extra_info_status, kphp::rpc::response_extra_info>> rpc_responses_extra_info;

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

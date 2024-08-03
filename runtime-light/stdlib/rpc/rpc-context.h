// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-extra-info.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/tl/tl-core.h"

struct RpcComponentContext final : private vk::not_copyable {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  tl::TLBuffer rpc_buffer;
  int64_t current_query_id{0};
  CurrentTlQuery current_query;
  unordered_map<int64_t, int64_t> response_waiters_forks;
  unordered_map<int64_t, class_instance<RpcTlQuery>> response_fetcher_instances;
  unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>> rpc_responses_extra_info;

  explicit RpcComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
    : current_query()
    , response_waiters_forks(unordered_map<int64_t, int64_t>::allocator_type{memory_resource})
    , response_fetcher_instances(unordered_map<int64_t, class_instance<RpcTlQuery>>::allocator_type{memory_resource})
    , rpc_responses_extra_info(
        unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>>::allocator_type{memory_resource}) {}

  static RpcComponentContext &get() noexcept;
};

// ================================================================================================

struct RpcImageState final : private vk::not_copyable {
  array<tl_storer_ptr> tl_storers_ht;
  tl_fetch_wrapper_ptr tl_fetch_wrapper{nullptr};

  static const RpcImageState &get() noexcept;
  static RpcImageState &get_mutable() noexcept;
};

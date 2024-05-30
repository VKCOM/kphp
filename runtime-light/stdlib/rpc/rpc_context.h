// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/rpc/rpc_extra_info.h"
#include "runtime-light/streams/component_stream.h"
#include "runtime-light/tl/tl_defs.h"
#include "runtime-light/tl/tl_rpc_query.h"

struct RpcComponentContext final : private vk::not_copyable {
  class FetchState {
    size_t m_pos{0};
    size_t m_remaining{0};

  public:
    constexpr FetchState() = default;

    constexpr size_t remaining() const noexcept {
      return m_remaining;
    }
    constexpr size_t pos() const noexcept {
      return m_pos;
    }
    constexpr void reset(size_t pos, size_t len) noexcept {
      m_pos = pos;
      m_remaining = len;
    }
    constexpr void adjust(size_t len) noexcept {
      m_pos += len;
      m_remaining -= len;
    }
  };

  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  string_buffer buffer;
  FetchState fetch_state;

  int64_t current_query_id{0};
  CurrentTlQuery current_query;
  unordered_map<int64_t, class_instance<C$ComponentQuery>> pending_component_queries;
  unordered_map<int64_t, class_instance<RpcTlQuery>> pending_rpc_queries;
  unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>> rpc_responses_extra_info;

  explicit RpcComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource)
    : current_query()
    , pending_component_queries(unordered_map<int64_t, class_instance<C$ComponentQuery>>::allocator_type{memory_resource})
    , pending_rpc_queries(unordered_map<int64_t, class_instance<RpcTlQuery>>::allocator_type{memory_resource})
    , rpc_responses_extra_info(
        unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>>::allocator_type{memory_resource}) {}
};

struct RpcImageState final : private vk::not_copyable {
  array<tl_storer_ptr> tl_storers_ht;
  tl_fetch_wrapper_ptr tl_fetch_wrapper{nullptr};
};

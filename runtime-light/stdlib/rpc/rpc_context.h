// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>

#include "common/mixin/not_copyable.h"
#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/stdlib/rpc/rpc_extra_info.h"
#include "runtime-light/streams/component_stream.h"
#include "runtime-light/tl/tl_defs.h"
#include "runtime-light/tl/tl_rpc_query.h"

struct RpcComponentContext final : private vk::not_copyable {
  class FetchState {
    size_t pos_{0};
    size_t len_{0};

  public:
    constexpr FetchState() = default;

    constexpr size_t len() const noexcept {
      return len_;
    }

    constexpr size_t pos() const noexcept {
      return pos_;
    }

    constexpr void reset(size_t pos, size_t len) noexcept {
      pos_ = pos;
      len_ = len;
    }

    constexpr void adjust(size_t len) noexcept {
      pos_ += len;
      len_ -= len;
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

  array<tl_storer_ptr> tl_storers_ht;             // TODO: move to image state
  tl_fetch_wrapper_ptr tl_fetch_wrapper{nullptr}; // TODO: move to image state

  explicit RpcComponentContext(memory_resource::unsynchronized_pool_resource &script_allocator) // TODO: think about allocator
    : vk::not_copyable()
    , current_query()
    , pending_component_queries(unordered_map<int64_t, class_instance<C$ComponentQuery>>::allocator_type{script_allocator})
    , pending_rpc_queries(unordered_map<int64_t, class_instance<RpcTlQuery>>::allocator_type{script_allocator})
    , rpc_responses_extra_info(
        unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>>::allocator_type{script_allocator}) {}
};

struct RpcImageState final : private vk::not_copyable {
  array<tl_storer_ptr> tl_storers_ht;
  tl_fetch_wrapper_ptr tl_fetch_wrapper;
};

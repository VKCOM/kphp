//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-light/allocator/memory_resource/resource_allocator.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/streams/interface.h"
#include "runtime-light/streams/streams.h"

struct JobWorkerClientContext : private vk::not_copyable {
  template<typename Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  int64_t current_job_id{0};
  unordered_map<int64_t, class_instance<C$ComponentQuery>> pending_queries;

  explicit JobWorkerClientContext(memory_resource::unsynchronized_pool_resource &memory_resource)
    : pending_queries(unordered_map<int64_t, class_instance<C$ComponentQuery>>::allocator_type{memory_resource}) {}
};

struct JobWorkerServerContext : private vk::not_copyable {
  class_instance<C$ComponentStream> stream_to_client;
};

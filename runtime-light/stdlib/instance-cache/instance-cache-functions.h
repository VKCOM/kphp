// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/visitors/instance-deep-copy-visitor.h"
#include "runtime-light/stdlib/visitors/instance-deep-estimate-size-visitor.h"

// shared memory layout: class_name_hash(u64) | class_instance shell | inner data
template<typename InstanceType>
bool f$instance_cache_store(const string& key, class_instance<InstanceType> instance, int64_t ttl = 0) noexcept {
  if (key.empty()) [[unlikely]] {
    kphp::log::warning("instance_cache_store. empty key is not supported");
    return false;
  }
  if (instance.is_null()) [[unlikely]] {
    kphp::log::warning("instance_cache_store. can't store a null instance: key -> {}", key.c_str());
    return false;
  }
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("instance_cache_store. ttl less than 0, key will be stored forever: ttl -> {}, key -> {}", ttl, key.c_str());
    ttl = 0;
  }
  if (constexpr int64_t max_ttl{std::numeric_limits<int64_t>::max() / 1000}; ttl > max_ttl) [[unlikely]] {
    kphp::log::warning("instance_cache_store. ttl is too large, key will be stored forever: ttl -> {}, max ttl -> {}, key -> {}", ttl, max_ttl, key.c_str());
    ttl = 0;
  }

  kphp::visitors::instance_deep_estimate_size_visitor estimate_size_visitor{};
  if (!estimate_size_visitor.process_instance(instance)) [[unlikely]] {
    kphp::log::warning("instance_cache_store. failed to estimate instance size: key -> {}", key.c_str());
    return false;
  }
  const size_t estimated_size{estimate_size_visitor.get_estimated_size()};
  constexpr size_t instance_size{sizeof(class_instance<InstanceType>)};
  constexpr size_t hash_size{sizeof(uint64_t)};

  auto alloc_result{k2::alloc_shared_memory(hash_size + instance_size + estimated_size, alignof(std::max_align_t))};
  if (!alloc_result.has_value()) [[unlikely]] {
    kphp::log::warning("instance_cache_store. failed to allocate shared memory: error -> {}, key -> {}", alloc_result.error(), key.c_str());
    return false;
  }
  std::byte* mem{static_cast<std::byte*>(alloc_result.value())};

  const uint64_t class_name_hash{InstanceType::CLASS_NAME_HASH};
  std::memcpy(mem, &class_name_hash, hash_size);
  // deep-copies the object graph into the inner area and rewrites the instance's fields to point at the copies,
  // so the whole graph ends up inside the shared-memory block.
  // All copies are pinned with ExtraRefCnt::for_instance_cache and are never freed individually -- the platform owns the block.
  kphp::visitors::instance_deep_copy_visitor copy_visitor{std::span{mem + hash_size + instance_size, estimated_size}, ExtraRefCnt::for_instance_cache};
  if (!copy_visitor.process_instance(instance)) [[unlikely]] {
    // estimate_size_visitor and copy_visitor must stay in sync, so this should never actually happen.
    // If this warning ever fires, it's a bug in one of the two visitors -- the allocated block above is leaked.
    kphp::log::warning("instance_cache_store. failed to deep-copy instance into shared memory: estimated size -> {}, key -> {}", estimated_size, key.c_str());
    return false;
  }
  std::construct_at(reinterpret_cast<class_instance<InstanceType>*>(mem + hash_size), std::move(instance));

  // the platform expects ttl in milliseconds, while the PHP API accepts seconds
  if (auto publish_result{k2::publish_shared_memory(std::string_view{key.c_str(), key.size()}, mem, ttl * 1000, false, true)}; publish_result.has_value()) {
    InstanceCacheInstanceState::get().request_cache.insert_or_assign(key, std::span{mem, hash_size + instance_size + estimated_size});
    return true;
  } else {
    // publish is expected to always succeed here (ignore_if_exist=true, valid key/memory), so this should never actually happen.
    // If this warning ever fires, the allocated block above is leaked, since it's never published and thus never reclaimed.
    kphp::log::warning("instance_cache_store. failed to publish shared memory: error -> {}, key -> {}", publish_result.error(), key.c_str());
    return false;
  }
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string& class_name, const string& key, bool /* even_if_expired */ = false) noexcept {
  static_assert(is_class_instance_v<ClassInstanceType>, "class_instance<> type expected");
  constexpr size_t hash_size{sizeof(uint64_t)};
  constexpr size_t instance_size{sizeof(ClassInstanceType)};

  if (key.empty()) [[unlikely]] {
    kphp::log::warning("instance_cache_fetch. empty key is not supported");
    return {};
  }
  // unwraps and validates a shared memory block: returns a null instance if the block is malformed or belongs to another class
  const auto unwrap{[&class_name, &key](std::span<const std::byte> mem) noexcept -> ClassInstanceType {
    if (mem.size() < hash_size + instance_size) [[unlikely]] {
      kphp::log::warning("instance_cache_fetch. shared memory is too small: size -> {}, expected at least -> {}, key -> {}", mem.size(),
                         hash_size + instance_size, key.c_str());
      return {};
    }

    uint64_t stored_class_name_hash{};
    std::memcpy(&stored_class_name_hash, mem.data(), sizeof(stored_class_name_hash));

    if (stored_class_name_hash != vk::murmur_hash<uint64_t>(class_name.c_str(), class_name.size())) [[unlikely]] {
      kphp::log::warning("instance_cache_fetch. trying to fetch incompatible instance class: class -> {}, key -> {}", class_name.c_str(), key.c_str());
      return {};
    }

    return *reinterpret_cast<const ClassInstanceType*>(mem.data() + hash_size);
  }};

  auto& request_cache{InstanceCacheInstanceState::get().request_cache};
  if (auto it{request_cache.find(key)}; it != request_cache.end()) {
    return unwrap(it->second);
  }

  auto get_result{k2::get_shared_memory(std::string_view{key.c_str(), key.size()}, true)};
  if (!get_result.has_value()) {
    return {};
  }
  request_cache.insert_or_assign(key, get_result.value());
  return unwrap(get_result.value());
}

inline bool f$instance_cache_update_ttl(const string& key, int64_t ttl = 0) noexcept {
  if (key.empty()) [[unlikely]] {
    kphp::log::warning("instance_cache_update_ttl. empty key is not supported");
    return false;
  }
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("instance_cache_update_ttl. ttl less than 0, key will be stored forever: ttl -> {}, key -> {}", ttl, key.c_str());
    ttl = 0;
  }
  if (constexpr int64_t max_ttl{std::numeric_limits<int64_t>::max() / 1000}; ttl > max_ttl) [[unlikely]] {
    kphp::log::warning("instance_cache_update_ttl. ttl is too large, key will be stored forever: ttl -> {}, max ttl -> {}, key -> {}", ttl, max_ttl,
                       key.c_str());
    ttl = 0;
  }
  // the platform expects ttl in milliseconds, while the PHP API accepts seconds
  return k2::update_ttl_shared_memory(std::string_view{key.c_str(), key.size()}, ttl * 1000).has_value();
}

inline bool f$instance_cache_delete(const string& key) noexcept {
  if (key.empty()) [[unlikely]] {
    kphp::log::warning("instance_cache_delete. empty key is not supported");
    return false;
  }
  InstanceCacheInstanceState::get().request_cache.erase(key);
  return k2::expire_shared_memory(std::string_view{key.c_str(), key.size()}).has_value();
}

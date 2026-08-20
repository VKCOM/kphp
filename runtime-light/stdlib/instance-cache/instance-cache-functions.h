// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"
#include "runtime-light/stdlib/instance-cache/visitors/instance-deep-copy-visitor.h"
#include "runtime-light/stdlib/instance-cache/visitors/instance-deep-size-count-visitor.h"

// shared memory layout: class_name_hash(u64) | class_instance shell | inner data
// (deep copies of all arrays, strings and nested instance bodies reachable from the instance);
// the hash goes first so that fetch can verify the requested class without touching the instance itself
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

  kphp::visitors::instance_deep_size_count_visitor instance_deep_size_count_visitor{};
  instance_deep_size_count_visitor.process_instance(instance);
  size_t inner_size{instance_deep_size_count_visitor.get_inner_size()};
  constexpr size_t instance_size{sizeof(class_instance<InstanceType>)};
  constexpr size_t hash_size{sizeof(uint64_t)};

  auto alloc_result{k2::alloc_shared_memory(hash_size + instance_size + inner_size, alignof(std::max_align_t))};
  if (!alloc_result.has_value()) [[unlikely]] {
    kphp::log::warning("instance_cache_store. failed to allocate shared memory: error -> {}, key -> {}", alloc_result.error(), key.c_str());
    return false;
  }
  std::byte* mem{static_cast<std::byte*>(alloc_result.value())};

  const uint64_t class_name_hash{static_cast<uint64_t>(InstanceType::CLASS_NAME_HASH)};
  std::memcpy(mem, &class_name_hash, hash_size);
  // deep-copies the object graph into the inner area and rewrites the instance's fields to point at the copies,
  // so the whole graph ends up inside the shared-memory block; all copies are pinned with ExtraRefCnt::for_instance_cache
  // and are never freed individually — the platform owns the block
  kphp::visitors::instance_deep_copy_visitor{std::span{mem + hash_size + instance_size, inner_size}, ExtraRefCnt::for_instance_cache}.process_instance(
      instance);
  std::construct_at(reinterpret_cast<class_instance<InstanceType>*>(mem + hash_size), std::move(instance));

  // the platform expects ttl in milliseconds, while the PHP API accepts seconds
  if (auto publish_result{k2::publish_shared_memory(std::string_view{key.c_str(), key.size()}, mem, ttl * 1000, false, true)}; publish_result.has_value()) {
    InstanceCacheInstanceState::get().request_cache.insert_or_assign(key, std::span{mem, hash_size + instance_size + inner_size});
    return true;
  } else {
    kphp::log::warning("instance_cache_store. failed to publish shared memory: error -> {}, key -> {}", publish_result.error(), key.c_str());
    return false;
  }
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string& class_name, const string& key, bool /* even_if_expired */ = false) noexcept {
  static_assert(is_class_instance_v<ClassInstanceType>, "class_instance<> type expected");
  constexpr size_t hash_size{sizeof(uint64_t)};
  constexpr size_t instance_size{sizeof(ClassInstanceType)};

  std::span<const std::byte> mem{};

  auto& request_cache{InstanceCacheInstanceState::get().request_cache};
  if (auto it{request_cache.find(key)}; it != request_cache.end()) {
    mem = it->second;
  } else if (auto get_result{k2::get_shared_memory(std::string_view{key.c_str(), key.size()})}; get_result.has_value()) {
    mem = get_result.value();
  } else {
    return {};
  }

  if (mem.size() < hash_size + instance_size) [[unlikely]] {
    // the shared memory under this key was not written by instance_cache_store for this class (or is corrupted)
    kphp::log::warning("instance_cache_fetch. shared memory is too small: size -> {}, expected at least -> {}, key -> {}", mem.size(),
                       hash_size + instance_size, key.c_str());
    return {};
  }

  uint64_t stored_class_name_hash{};
  std::memcpy(&stored_class_name_hash, mem.data(), sizeof(stored_class_name_hash));

  if (stored_class_name_hash != vk::murmur_hash<uint64_t>(class_name.c_str(), class_name.size())) {
    kphp::log::warning("instance_cache_fetch. requested class doesn't match the stored one: requested -> {}, key -> {}", class_name.c_str(), key.c_str());
    return {};
  }

  request_cache.insert_or_assign(key, mem);
  return *reinterpret_cast<const ClassInstanceType*>(mem.data() + hash_size);
}

inline bool f$instance_cache_update_ttl(const string& key, int64_t ttl = 0) noexcept {
  if (ttl < 0) [[unlikely]] {
    kphp::log::warning("instance_cache_update_ttl. ttl less than 0, key will be stored forever: ttl -> {}, key -> {}", ttl, key.c_str());
    ttl = 0;
  }
  // the platform expects ttl in milliseconds, while the PHP API accepts seconds
  return k2::update_ttl_shared_memory(std::string_view{key.c_str(), key.size()}, ttl * 1000).has_value();
}

inline bool f$instance_cache_delete(const string& key) noexcept {
  InstanceCacheInstanceState::get().request_cache.erase(key);
  return k2::delete_shared_memory(std::string_view{key.c_str(), key.size()}).has_value();
}

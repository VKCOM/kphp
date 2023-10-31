// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// This API inspired by APC and allow to cache any kind of instances between requests and workers.
// Highlights:
//  1) All strings, arrays, instances are placed into common shared between workers memory;
//  2) On store, all constant strings and arrays (check ExtraRefCnt::for_global_const) are shallow copied as is,
//    otherwise, a deep copy is used and the reference counter is set to special value (check ExtraRefCnt::for_instance_cache),
//    therefore the reference counter of cached strings and arrays is ExtraRefCnt::for_instance_cache or ExtraRefCnt::for_global_const;
//  3) On fetch, all strings and arrays are returned as is;
//  4) On store, all instances (and sub instances) are deeply cloned into instance cache;
//  5) On fetch, all instances (and sub instances) are deeply cloned from instance cache;
//  6) All instances (with all members) are destroyed strictly before or after request,
//    and shouldn't be destroyed while request.

#include "common/mixin/not_copyable.h"

#include "memory_usage.h"
#include "runtime/instance-copy-processor.h"
#include "runtime/kphp_core.h"
#include "runtime/shape.h"
#include "server/statshouse/statshouse-manager.h"

enum class InstanceCacheStoreStatus;

namespace impl_ {

std::string_view instance_cache_store_status_to_str(InstanceCacheStoreStatus status);
InstanceCacheStoreStatus instance_cache_store(const string &key, const InstanceCopyistBase &instance_wrapper, int64_t ttl);
const InstanceCopyistBase *instance_cache_fetch_wrapper(const string &key, bool even_if_expired);

} // namespace impl_

void global_init_instance_cache_lib();
void init_instance_cache_lib();
void free_instance_cache_lib();

// these function should be called from master
void set_instance_cache_memory_limit(size_t limit);

struct InstanceCacheStats : private vk::not_copyable {
  std::atomic<uint64_t> elements_stored{0};
  std::atomic<uint64_t> elements_stored_with_delay{0};
  std::atomic<uint64_t> elements_storing_skipped_due_recent_update{0};
  std::atomic<uint64_t> elements_storing_delayed_due_mutex{0};

  std::atomic<uint64_t> elements_fetched{0};
  std::atomic<uint64_t> elements_missed{0};
  std::atomic<uint64_t> elements_missed_earlier{0};

  std::atomic<uint64_t> elements_expired{0};
  std::atomic<uint64_t> elements_logically_expired_but_fetched{0};
  std::atomic<uint64_t> elements_logically_expired_and_ignored{0};
  std::atomic<uint64_t> elements_created{0};
  std::atomic<uint64_t> elements_destroyed{0};
  std::atomic<uint64_t> elements_cached{0};
};

enum class InstanceCacheSwapStatus {
  no_need, // no need to do a swap
  swap_is_finished, // swap succeeded
  swap_is_forbidden // swap is not possible - the memory is still being used
};

enum class InstanceCacheStoreStatus {
  success,
  skipped,
  memory_limit_exceeded,
  delayed,
  failed
};

// these function should be called from master
InstanceCacheSwapStatus instance_cache_try_swap_memory();
// these function should be called from master
const InstanceCacheStats &instance_cache_get_stats();
// these function should be called from master
const memory_resource::MemoryStats &instance_cache_get_memory_stats();
// these function should be called from master
void instance_cache_purge_expired_elements();

void instance_cache_release_all_resources_acquired_by_this_proc();

template<typename ClassInstanceType>
void send_extended_instance_cache_stats_if_enabled(std::string_view op, InstanceCacheStoreStatus status, const string &key, const ClassInstanceType &instance) {
  if (StatsHouseManager::get().is_extended_instance_cache_stats_enabled()) {
    int64_t size = f$estimate_memory_usage(key) + f$estimate_memory_usage(instance);
    StatsHouseManager::get().add_extended_instance_cache_stats(op, impl_::instance_cache_store_status_to_str(status), key, size);
  }
}

template<typename ClassInstanceType>
bool f$instance_cache_store(const string &key, const ClassInstanceType &instance, int64_t ttl = 0) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (instance.is_null()) {
    return false;
  }
  InstanceCopyistImpl<ClassInstanceType> instance_wrapper{instance};
  InstanceCacheStoreStatus status = impl_::instance_cache_store(key, instance_wrapper, ttl);
  send_extended_instance_cache_stats_if_enabled("store", status, key, instance);
  return status == InstanceCacheStoreStatus::success;
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string &class_name, const string &key, bool even_if_expired = false) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (const auto *base_wrapper = impl_::instance_cache_fetch_wrapper(key, even_if_expired)) {
    // do not use first parameter (class name) for verifying type,
    // because different classes from separated libs may have same names
    if (auto wrapper = dynamic_cast<const InstanceCopyistImpl<ClassInstanceType> *>(base_wrapper)) {
      auto result = wrapper->get_instance();
      php_assert(!result.is_null());
      send_extended_instance_cache_stats_if_enabled("fetch", InstanceCacheStoreStatus::success, key, result);
      return result;
    } else {
      php_warning("Trying to fetch incompatible instance class: expect '%s', got '%s'",
                  class_name.c_str(), base_wrapper->get_class());
      ClassInstanceType empty = {};
      send_extended_instance_cache_stats_if_enabled("fetch", InstanceCacheStoreStatus::failed, key, empty);
    }
  }
  return {};
}

bool f$instance_cache_update_ttl(const string &key, int64_t ttl = 0);
bool f$instance_cache_delete(const string &key);

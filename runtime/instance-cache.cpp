// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/instance-cache.h"

#include <chrono>
#include <forward_list>
#include <map>
#include <mutex>
#include <unordered_set>

#include "common/kprintf.h"
#include "common/wrappers/memory-utils.h"

#include "kphp-core/class-instance/refcountable_php_classes.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/inter-process-mutex.h"
#include "runtime/inter-process-resource.h"
#include "runtime/memory_resource/resource_allocator.h"

namespace impl_ {

//#define DEBUG_INSTANCE_CACHE

#ifdef DEBUG_INSTANCE_CACHE
  #define ic_debug kprintf
#else
inline void ic_debug(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
inline void ic_debug(const char *, ...) {}
#endif

// Default memory limit for the entire buffer (there are 2 buffers in total)
static constexpr size_t DEFAULT_MEMORY_LIMIT{256u * 1024u * 1024u};
// Buffer memory consumption threshold that states at which point we'll swap it
static constexpr double REAL_MEMORY_USED_THRESHOLD{0.9};
// Elements that lived less than this ratio to the expected lifetime will not be overwritten
static constexpr double FRESHNESS_ELEMENT_RATIO{0.2};
// For the element that lived more than this ratio to the expected lifetime,
// the next fetch will return null (only once), so the process that receives it could update it before it's completely removed
static constexpr double EARLY_EXPIRATION_ELEMENT_RATIO{0.8};
// An upper lifetime limit for an element after its removal (so it doesn't live too long)
static constexpr std::chrono::seconds DELETED_ELEMENT_LIFETIME_LIMIT{1};
// After the lifetime expiration, the element will be removed after this interval
static constexpr std::chrono::minutes PHYSICAL_REMOVING_DELAY{1};
// Number of buckets that are used for the elements sharding
static constexpr size_t DATA_SHARDS_COUNT{997u};
// The buckets check step during the cache cleanup
static constexpr size_t SHARDS_PURGE_PERIOD{5u};

class ElementHolder;

struct CacheContext : private vk::not_copyable {
  inter_process_mutex allocator_mutex;
  memory_resource::unsynchronized_pool_resource memory_resource;
  InstanceCacheStats stats;
  std::atomic<bool> memory_swap_required{false};

  void move_to_garbage(ElementHolder *element) noexcept;
  bool has_garbage() const noexcept { return cache_garbage_ != nullptr; }
  void clear_garbage() noexcept;

private:
  std::atomic<ElementHolder *> cache_garbage_{nullptr};
};

class ElementHolder : private vk::thread_safe_refcnt<ElementHolder> {
public:
  using vk::thread_safe_refcnt<ElementHolder>::add_ref;
  using vk::thread_safe_refcnt<ElementHolder>::get_refcnt;

  void release() noexcept {
    if (--refcnt == 0) {
      cache_context.move_to_garbage(this);
    }
  }

  void destroy() noexcept {
    php_assert(refcnt == 0);
    cache_context.stats.elements_destroyed.fetch_add(1, std::memory_order_relaxed);
    auto &mem_resource = cache_context.memory_resource;
    this->~ElementHolder();
    mem_resource.deallocate(this, sizeof(ElementHolder));
  }

  ElementHolder(std::chrono::nanoseconds now, int64_t ttl,
                std::unique_ptr<InstanceCopyistBase> &&instance,
                CacheContext &context) noexcept:
    inserted_by_process(getpid()),
    instance_wrapper(std::move(instance)),
    cache_context(context) {
    update_time_points(now, ttl);
    cache_context.stats.elements_created.fetch_add(1, std::memory_order_relaxed);
  }

  // returns how long the element is lived in relation to the expected lifetime
  double freshness_ratio(std::chrono::nanoseconds now, double immortal_ratio = 0.5) const noexcept {
    // an immortal element
    if (expiring_at == std::chrono::nanoseconds::max()) {
      return immortal_ratio;
    }
    if (expiring_at <= stored_at) {
      return 1.0;
    }
    const auto real_age = std::chrono::duration<double>{std::max(now, stored_at) - stored_at};
    const auto max_age = std::chrono::duration<double>{expiring_at - stored_at};
    return real_age.count() / max_age.count();
  }

  void update_time_points(std::chrono::nanoseconds now, int64_t ttl) noexcept {
    stored_at = std::max(now, stored_at);
    expiring_at = ttl > 0 ? stored_at + std::chrono::seconds{ttl} : std::chrono::nanoseconds::max();
    early_fetch_performed = false;
  }

  std::chrono::nanoseconds stored_at{std::chrono::nanoseconds::min()};
  std::chrono::nanoseconds expiring_at{std::chrono::nanoseconds::max()};
  bool early_fetch_performed{false};
  const pid_t inserted_by_process{0};

  std::unique_ptr<InstanceCopyistBase> instance_wrapper;
  CacheContext &cache_context;

  // Removed elements list
  std::atomic<ElementHolder *> next_in_garbage_list{nullptr};
};

using ElementStorage_ = memory_resource::stl::map<string, vk::intrusive_ptr<ElementHolder>, memory_resource::unsynchronized_pool_resource, stl_string_less>;

struct SharedDataStorages : private vk::not_copyable {
  explicit SharedDataStorages(memory_resource::unsynchronized_pool_resource &resource) :
    storage(ElementStorage_::allocator_type{resource}) {
  }

  inter_process_mutex storage_mutex;
  ElementStorage_ storage;
  std::atomic<bool> is_storage_empty{true};
};

void CacheContext::move_to_garbage(ElementHolder *element) noexcept {
  php_assert(element->next_in_garbage_list == nullptr);
  // Put all garbage into the cache_context.cache_garbage; the cleanup happens later, under the lock
  auto *next = cache_garbage_.load();
  do {
    element->next_in_garbage_list.store(next);
  } while (!cache_garbage_.compare_exchange_strong(next, element));
}

void CacheContext::clear_garbage() noexcept {
  auto *element = cache_garbage_.exchange(nullptr);

  while (element) {
    auto *next = element->next_in_garbage_list.load();
    element->destroy();
    element = next;
  }
}

class SharedMemoryData : vk::not_copyable {
public:
  void init(size_t pool_size) noexcept {
    php_assert(!data_shards_);
    php_assert(!cache_context_);
    php_assert(!shared_memory_);
    shared_memory_pool_size_ = pool_size;
    share_memory_full_size_ = get_context_size() + get_data_size() + shared_memory_pool_size_;
    shared_memory_ = mmap_shared(share_memory_full_size_);
    construct_data_inplace();
  }

  void reset() noexcept {
    destroy_data();
    construct_data_inplace();
  }

  void destroy() noexcept {
    destroy_data();
    data_shards_ = nullptr;
    cache_context_ = nullptr;
  }

  SharedDataStorages &get_data(const string &key) noexcept {
    php_assert(data_shards_);
    return data_shards_[static_cast<uint32_t>(key.hash()) % DATA_SHARDS_COUNT];
  }

  SharedDataStorages *get_data_shards() noexcept {
    return data_shards_;
  }

  static constexpr size_t get_data_shards_count() noexcept {
    return DATA_SHARDS_COUNT;
  }

  CacheContext &get_context() noexcept {
    php_assert(cache_context_);
    return *cache_context_;
  }

private:
  void destroy_data() noexcept {
    php_assert(data_shards_);
    for (size_t i = 0; i != DATA_SHARDS_COUNT; ++i) {
      // to avoid of calling all destructors, just free storage_mutex
      data_shards_[i].storage_mutex.~inter_process_mutex();
    }

    php_assert(cache_context_);
    cache_context_->~CacheContext();
  }

  void construct_data_inplace() noexcept {
    cache_context_ = new(shared_memory_) CacheContext();
    uint8_t *data_storage_mem = static_cast<uint8_t *>(shared_memory_) + get_context_size();
    cache_context_->memory_resource.init(data_storage_mem + get_data_size(), shared_memory_pool_size_);
    data_shards_ = reinterpret_cast<SharedDataStorages *>(data_storage_mem);
    for (size_t i = 0; i != DATA_SHARDS_COUNT; ++i) {
      new(&data_shards_[i]) SharedDataStorages{cache_context_->memory_resource};
    }
  }

  static constexpr size_t get_context_size() noexcept {
    return (sizeof(CacheContext) + 7) & -8;
  }

  static constexpr size_t get_data_size() noexcept {
    return (sizeof(SharedDataStorages) * DATA_SHARDS_COUNT + 7) & -8;
  }

  void *shared_memory_{nullptr};
  size_t share_memory_full_size_{0};
  size_t shared_memory_pool_size_{0};
  CacheContext *cache_context_{nullptr};
  SharedDataStorages *data_shards_{nullptr};
};

struct {
  size_t total_memory_limit{DEFAULT_MEMORY_LIMIT};
} static instance_cache_settings;

class InstanceCache {
private:
  InstanceCache() :
    now_(std::chrono::system_clock::now().time_since_epoch()) {
  }

public:
  static InstanceCache &get() {
    static InstanceCache cache;
    return cache;
  }

  void global_init() {
    php_assert(!current_ && !context_);
    data_manager_.init(instance_cache_settings.total_memory_limit);
  }

  void refresh() {
    php_assert(!current_ && !context_);
    update_now();
    current_ = data_manager_.acquire_current_resource();
    context_ = &current_->get_context();
  }

  void update_now() {
    now_ = std::chrono::system_clock::now().time_since_epoch();
  }

  void free() {
    php_assert(current_ && context_);
    sync_delayed();

    // request_cache_ and storing_delayed_ use a script memory
    storing_delayed_.clear();
    request_cache_.clear();
    // used_elements use a heap memory
    used_elements_.clear();

    if (context_->has_garbage()) {
      std::unique_lock<inter_process_mutex> allocator_lock{context_->allocator_mutex, std::try_to_lock};
      if (allocator_lock) {
        dl::MemoryReplacementGuard shared_memory_guard{context_->memory_resource};
        context_->clear_garbage();
      }
    }
    data_manager_.release_resource(current_);
    current_ = nullptr;
    context_ = nullptr;
  }

  InstanceCacheOpStatus store(const string &key, const InstanceCopyistBase &instance_wrapper, int64_t ttl) noexcept {
    ic_debug("store '%s'\n", key.c_str());
    php_assert(current_ && context_);
    if (context_->memory_swap_required) {
      return InstanceCacheOpStatus::memory_swap_required;
    }

    sync_delayed();
    // various service things that we can do without synchronization
    auto &data = current_->get_data(key);
    update_now();
    if (is_element_insertion_can_be_skipped(data, key)) {
      return InstanceCacheOpStatus::skipped;
    }

    InstanceDeepCopyVisitor detach_processor{context_->memory_resource, ExtraRefCnt::for_instance_cache};
    const ElementHolder *inserted_element = try_insert_element_into_cache(
      data, key, ttl, instance_wrapper, detach_processor);

    if (!inserted_element) {
      // failed to insert the element due to some problems (e.g. memory, depth limit)
      if (unlikely(!detach_processor.is_ok())) {
        if (detach_processor.is_memory_limit_exceeded()) {
          fire_warning(instance_wrapper.get_class());
          return InstanceCacheOpStatus::memory_limit_exceeded;
        }

        return InstanceCacheOpStatus::failed;
      }
      // failed to acquire a lock, save the instance into the script memory container, we'll try again later
      class_instance<DelayedInstance> delayed_instance;
      delayed_instance.alloc().get()->ttl = ttl;
      delayed_instance.get()->instance_wrapper = instance_wrapper.shallow_copy();
      storing_delayed_.set_value(key, std::move(delayed_instance));
      context_->stats.elements_storing_delayed_due_mutex.fetch_add(1, std::memory_order_relaxed);
      return InstanceCacheOpStatus::delayed;
    }
    ic_debug("element '%s' was successfully inserted\n", key.c_str());
    context_->stats.elements_stored.fetch_add(1, std::memory_order_relaxed);
    // request_cache_ uses a script memory
    request_cache_.set_value(key, inserted_element);
    return InstanceCacheOpStatus::success;
  }

  const InstanceCopyistBase *fetch(const string &key, bool even_if_expired) {
    php_assert(current_ && context_);
    sync_delayed();
    // storing_delayed_ uses a script memory
    if (const auto *delayed_instance = storing_delayed_.find_value(key)) {
      ic_debug("fetch '%s' from delayed cache\n", key.c_str());
      return delayed_instance->get()->instance_wrapper.get();
    }
    // request_cache_ uses a script memory
    if (const ElementHolder *const *cached_element_ptr = request_cache_.find_value(key)) {
      ic_debug("fetch '%s' from request cache\n", key.c_str());
      return (*cached_element_ptr)->instance_wrapper.get();
    }

    vk::intrusive_ptr<ElementHolder> element;
    bool element_logically_expired = false;
    {
      auto &data = current_->get_data(key);
      std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
      auto it = data.storage.find(key);
      if (it == data.storage.end()) {
        ic_debug("can't fetch '%s' because it is absent\n", key.c_str());
        context_->stats.elements_missed.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
      }

      update_now();
      // if more than EARLY_EXPIRATION_ELEMENT_RATIO time is passed out of the expected element lifetime,
      // return null to the next worker process so it knows that the value needs to be updated in advance
      if (!it->second->early_fetch_performed &&
          it->second->freshness_ratio(now_) >= EARLY_EXPIRATION_ELEMENT_RATIO) {
        it->second->early_fetch_performed = true;
        context_->stats.elements_missed_earlier.fetch_add(1, std::memory_order_relaxed);
        ic_debug("can't fetch '%s' because less than %f of total time is left\n",
                 key.c_str(), EARLY_EXPIRATION_ELEMENT_RATIO);
        return nullptr;
      }
      element_logically_expired = it->second->expiring_at <= now_;
      if (element_logically_expired) {
        if (even_if_expired) {
          context_->stats.elements_logically_expired_but_fetched.fetch_add(1, std::memory_order_relaxed);
          ic_debug("fetch logically expired element '%s'\n", key.c_str());
        } else {
          context_->stats.elements_logically_expired_and_ignored.fetch_add(1, std::memory_order_relaxed);
          ic_debug("can't fetch '%s' because element was logically expired\n", key.c_str());
          return nullptr;
        }
      } else {
        context_->stats.elements_fetched.fetch_add(1, std::memory_order_relaxed);
        ic_debug("fetch '%s' from inter process cache\n", key.c_str());
      }

      element = it->second;
    }

    // don't cache logically expired elements
    if (!element_logically_expired) {
      // request_cache_ uses a script memory
      request_cache_.set_value(key, element.get());
    }

    const auto *result = element->instance_wrapper.get();
    {
      // used_elements uses a heap memory, it'll hold an element until the end of the request
      dl::CriticalSectionGuard heap_guard;
      used_elements_.emplace(std::move(element));
    }
    return result;
  }

  bool update_ttl(const string &key, int64_t ttl) {
    php_assert(current_ && context_);
    ic_debug("update_ttl '%s', new ttl '%" PRIi64 "'\n", key.c_str(), ttl);
    sync_delayed();
    // storing_delayed_ uses a script memory
    // it's not obvious what to do with the storing_delayed_;
    // but perhaps we need to either clear it from this key or do nothing at all
    if (const auto *delayed_instance = storing_delayed_.find_value(key)) {
      delayed_instance->get()->ttl = ttl;
    }

    auto &data = current_->get_data(key);
    update_now();
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    if (it == data.storage.end()) {
      return false;
    }

    it->second->update_time_points(now_, ttl);
    return true;
  }

  bool del(const string &key) {
    php_assert(current_ && context_);
    ic_debug("delete '%s'\n", key.c_str());
    sync_delayed();
    // request_cache_ and storing_delayed_ use a script memory
    storing_delayed_.unset(key);
    request_cache_.unset(key);
    auto &data = current_->get_data(key);
    update_now();
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    if (it == data.storage.end()) {
      return false;
    }

    // calculate expiring_at in a way that the next fetch returns false
    constexpr double SCALE = 1.0 / EARLY_EXPIRATION_ELEMENT_RATIO;
    auto new_element_ttl = std::chrono::duration_cast<std::chrono::nanoseconds>((now_ - it->second->stored_at) * SCALE);
    auto new_expiring_at = std::chrono::duration_cast<std::chrono::nanoseconds>(it->second->stored_at + new_element_ttl);
    new_expiring_at = std::min(new_expiring_at, now_ + DELETED_ELEMENT_LIFETIME_LIMIT);
    it->second->expiring_at = std::max(new_expiring_at, it->second->stored_at);
    return true;
  }

  void force_release_all_resources() {
    data_manager_.force_release_all_resources();
  }

  // this function should be called only from master
  void purge_expired() {
    update_now();
    const auto now_with_delay = now_ - PHYSICAL_REMOVING_DELAY;

    auto &current_data = data_manager_.get_current_resource();
    auto &context = current_data.get_context();
    // replace the default script allocator
    // as this call happens from the master process
    // we need to explicitly activate and deactivate it
    dl::MemoryReplacementGuard shared_memory_guard{context.memory_resource, true};

    auto *data_shards = current_data.get_data_shards();
    const size_t shards_count = current_data.get_data_shards_count();
    for (size_t shard_id = purge_shard_offset_; shard_id < shards_count; shard_id += SHARDS_PURGE_PERIOD) {
      auto &data_shard = data_shards[shard_id];
      if (data_shard.is_storage_empty.load(std::memory_order_relaxed)) {
        continue;
      }
      {
        std::lock_guard<inter_process_mutex> shared_data_lock{data_shard.storage_mutex};
        if (std::none_of(data_shard.storage.begin(), data_shard.storage.end(),
                         [now_with_delay](const auto &stored_element) {
                           return stored_element.second->expiring_at <= now_with_delay;
                         })) {
          continue;
        }
      }

      // lock in this very order and do not move allocator_lock anywhere below, otherwise it will result in a deadlock!
      std::lock_guard<inter_process_mutex> allocator_lock{context.allocator_mutex};
      std::lock_guard<inter_process_mutex> shared_data_lock{data_shard.storage_mutex};
      for (auto it = data_shard.storage.begin(); it != data_shard.storage.end();) {
        if (it->second->expiring_at <= now_with_delay) {
          ic_debug("purge '%s'\n", it->first.c_str());
          string removing_key = it->first;
          it = data_shard.storage.erase(it);
          InstanceDeepDestroyVisitor{ExtraRefCnt::for_instance_cache}.process(removing_key);
          context.stats.elements_expired.fetch_add(1, std::memory_order_relaxed);
          context.stats.elements_cached.fetch_sub(1, std::memory_order_relaxed);
        } else {
          ++it;
        }
      }
      data_shard.is_storage_empty.store(data_shard.storage.empty(), std::memory_order_relaxed);
    }

    purge_shard_offset_ = (purge_shard_offset_ + 1) % SHARDS_PURGE_PERIOD;

    std::lock_guard<inter_process_mutex> allocator_lock{context.allocator_mutex};
    context.clear_garbage();
    last_memory_stats_ = context.memory_resource.get_memory_stats();
  }

  // this function should be called only from master
  InstanceCacheSwapStatus try_swap_memory_resource() {
    const auto &memory_stats = get_last_memory_stats();
    const auto threshold = REAL_MEMORY_USED_THRESHOLD * static_cast<double>(memory_stats.memory_limit);
    if (static_cast<double>(memory_stats.real_memory_used) < threshold &&
        !data_manager_.get_current_resource().get_context().memory_swap_required) {
      return InstanceCacheSwapStatus::no_need;
    }
    return data_manager_.try_switch_to_next_unused_resource()
           ? InstanceCacheSwapStatus::swap_is_finished
           : InstanceCacheSwapStatus::swap_is_forbidden;
  }

  // this function should be called only from master
  const InstanceCacheStats &get_stats() {
    return data_manager_.get_current_resource().get_context().stats;
  }

  // this function should be called only from master
  const memory_resource::MemoryStats &get_last_memory_stats() const noexcept {
    return last_memory_stats_;
  }

private:
  bool is_element_insertion_can_be_skipped(SharedDataStorages &data, const string &key) const {
    std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
    auto it = data.storage.find(key);
    // allow to skip the insertion of the element if it was inserted by another process recently enough
    if (it != data.storage.end() &&
        it->second->freshness_ratio(now_) < FRESHNESS_ELEMENT_RATIO &&
        it->second->inserted_by_process != getpid()) {
      ic_debug("skip '%s' because it was recently updated\n", key.c_str());
      context_->stats.elements_storing_skipped_due_recent_update.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

  void sync_delayed() noexcept {
    php_assert(current_ && context_);
    if (context_->memory_swap_required) {
      return;
    }

    InstanceDeepCopyVisitor detach_processor{context_->memory_resource, ExtraRefCnt::for_instance_cache};
    for (auto it = storing_delayed_.cbegin(); it != storing_delayed_.cend(); it = storing_delayed_.cbegin()) {
      string key = it.get_key().to_string();
      const auto &delayed_instance = *it.get_value().get();
      auto &data = current_->get_data(key);
      update_now();
      if (is_element_insertion_can_be_skipped(data, key)) {
        storing_delayed_.unset(key);
        continue;
      }
      const ElementHolder *inserted_element = try_insert_element_into_cache(
        data, key, delayed_instance.ttl,
        *delayed_instance.instance_wrapper, detach_processor);
      if (!inserted_element) {
        if (likely(detach_processor.is_ok())) {
          // failed to acquire an allocator lock; try later
          return;
        }
        if (detach_processor.is_memory_limit_exceeded()) {
          fire_warning(delayed_instance.instance_wrapper->get_class());
          return;
        }
      } else {
        ic_debug("element '%s' was successfully inserted with delay\n", key.c_str());
        context_->stats.elements_stored_with_delay.fetch_add(1, std::memory_order_relaxed);
        // request_cache_ uses script memory
        request_cache_.set_value(key, inserted_element);
      }
      storing_delayed_.unset(key);
    }
  }

  ElementHolder *try_insert_element_into_cache(SharedDataStorages &data,
                                               const string &key_in_script_memory, int64_t ttl,
                                               const InstanceCopyistBase &instance_wrapper,
                                               InstanceDeepCopyVisitor &detach_processor) noexcept {
    // swap the allocator
    dl::MemoryReplacementGuard shared_memory_guard{context_->memory_resource};

    std::unique_lock<inter_process_mutex> allocator_lock{context_->allocator_mutex, std::try_to_lock};
    // locking strictly before the storage_mutex to avoid a deadlock
    if (!allocator_lock) {
      return nullptr;
    }

    // acquired an allocator lock, now we can safely collect the garbage
    auto clear_garbage = vk::finally([this] { context_->clear_garbage(); });

    // moving an instance into a shared memory
    if (auto cached_instance_wrapper = instance_wrapper.deep_copy_and_set_ref_cnt(detach_processor)) {
      if (void *mem = detach_processor.prepare_raw_memory(sizeof(ElementHolder))) {
        vk::intrusive_ptr<ElementHolder> element{new(mem) ElementHolder{now_, ttl, std::move(cached_instance_wrapper), *context_}};
        std::lock_guard<inter_process_mutex> shared_data_lock{data.storage_mutex};
        auto it = data.storage.find(key_in_script_memory);
        if (it == data.storage.end()) {
          string key_in_shared_memory = key_in_script_memory;
          if (unlikely(!detach_processor.process(key_in_shared_memory))) {
            return nullptr;
          }
          constexpr auto node_max_size = ElementStorage_::allocator_type::max_value_type_size();
          if (unlikely(!detach_processor.is_enough_memory_for(node_max_size))) {
            InstanceDeepDestroyVisitor{ExtraRefCnt::for_instance_cache}.process(key_in_shared_memory);
            return nullptr;
          }

          it = data.storage.emplace(std::move(key_in_shared_memory), vk::intrusive_ptr<ElementHolder>{}).first;
          data.is_storage_empty.store(false, std::memory_order_relaxed);
          context_->stats.elements_cached.fetch_add(1, std::memory_order_relaxed);
        }
        // replace element and save previous element into used_elements_;
        // it'll make it possible to free it without taking a storage_mutex lock
        it->second.swap(element);
        {
          dl::CriticalSectionGuard heap_guard;
          if (element) {
            // used_elements_ uses heap memory for its internal allocations
            used_elements_.emplace(std::move(element));
          }
          used_elements_.emplace(it->second);
        }
        return it->second.get();
      }
    }
    return nullptr;
  }

  void fire_warning(const char *class_name) noexcept {
    php_warning("Memory limit exceeded on saving instance of class '%s' into cache", class_name);
    context_->memory_swap_required = true;
  }

  SharedMemoryData *current_{nullptr};
  CacheContext *context_{nullptr};
  InterProcessResourceManager<SharedMemoryData, 2> data_manager_;


  struct IntrusivePtrHash {
    size_t operator()(const vk::intrusive_ptr<ElementHolder> &el) const {
      return reinterpret_cast<size_t>(el.get());
    }
  };

  // A storage of elements that were used inside a script (during the request)
  // Elements are inserted here to ensure that they don't go away unexpectedly
  // std::unordered_set uses heap memory
  // vk::intrusive_ptr<ElementHolder> uses shared memory
  // during script execution MUST be used only under critical section
  std::unordered_set<vk::intrusive_ptr<ElementHolder>, IntrusivePtrHash> used_elements_;

  // A local cache that can be used to get elements without taking a storage_mutex lock
  // Uses a script memory
  array<const ElementHolder *> request_cache_;

  // A container for instances which we failed to save from the first try due to the allocator lock
  // Uses script memory. Wrapped into the class_instance as array doesn't work with unique_ptr
  struct DelayedInstance : refcountable_php_classes<DelayedInstance> {
    std::unique_ptr<InstanceCopyistBase> instance_wrapper;
    int64_t ttl{0};
  };
  array<class_instance<DelayedInstance>> storing_delayed_;

  std::chrono::nanoseconds now_{std::chrono::nanoseconds::zero()};
  memory_resource::MemoryStats last_memory_stats_;
  size_t purge_shard_offset_{0};
};

std::string_view instance_cache_store_status_to_str(InstanceCacheOpStatus status) {
  switch (status) {
    case InstanceCacheOpStatus::success:                 return "success";
    case InstanceCacheOpStatus::skipped:                 return "skipped";
    case InstanceCacheOpStatus::memory_limit_exceeded:   return "memory_limit_exceeded";
    case InstanceCacheOpStatus::delayed:                 return "delayed";
    case InstanceCacheOpStatus::not_found:               return "not_found";
    case InstanceCacheOpStatus::failed:                  return "failed";
    default:                                             return "unknown";
  }
}

InstanceCacheOpStatus instance_cache_store(const string &key, const InstanceCopyistBase &instance_wrapper, int64_t ttl) {
  return InstanceCache::get().store(key, instance_wrapper, ttl);
}

const InstanceCopyistBase *instance_cache_fetch_wrapper(const string &key, bool even_if_expired) {
  return InstanceCache::get().fetch(key, even_if_expired);
}

} // namespace impl_

void global_init_instance_cache_lib() {
  impl_::InstanceCache::get().global_init();
}

void init_instance_cache_lib() {
  impl_::InstanceCache::get().refresh();
}

void free_instance_cache_lib() {
  impl_::InstanceCache::get().free();
}

// should be called only from master
void set_instance_cache_memory_limit(size_t limit) {
  impl_::instance_cache_settings.total_memory_limit = limit;
}

// should be called only from master
InstanceCacheSwapStatus instance_cache_try_swap_memory() {
  return impl_::InstanceCache::get().try_swap_memory_resource();
}

// should be called only from master
const InstanceCacheStats &instance_cache_get_stats() {
  return impl_::InstanceCache::get().get_stats();
}

// should be called only from master
const memory_resource::MemoryStats &instance_cache_get_memory_stats() {
  return impl_::InstanceCache::get().get_last_memory_stats();
}

// should be called only from master
void instance_cache_purge_expired_elements() {
  impl_::InstanceCache::get().purge_expired();
}

void instance_cache_release_all_resources_acquired_by_this_proc() {
  impl_::InstanceCache::get().force_release_all_resources();
}

bool f$instance_cache_update_ttl(const string &key, int64_t ttl) {
  return impl_::InstanceCache::get().update_ttl(key, ttl);
}

bool f$instance_cache_delete(const string &key) {
  return impl_::InstanceCache::get().del(key);
}

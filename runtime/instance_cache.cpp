#include "runtime/instance_cache.h"

#include "runtime/allocator.h"
#include "common/smart_ptrs/make_unique.h"

#include <chrono>
#include <forward_list>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace ic_impl_ {

DeepSharedDetach::DeepSharedDetach() :
  Basic(*this) {
}

DeepDestroy::DeepDestroy() :
  Basic(*this) {
}

DeepInstanceClone::DeepInstanceClone() :
  Basic(*this) {
}

bool DeepSharedDetach::process(string &str) {
  if (is_memory_limit_exceeded()) {
    str = string();
    return false;
  }

  if (!str.is_const_reference_counter()) {
    str.make_not_shared();
  }

  // make_not_shared may make str constant again (e.g. const empty or single char str), therefore check again
  if (!str.is_const_reference_counter()) {
    str.set_reference_counter_to_cache();
  }
  return true;
}

bool DeepDestroy::process(string &str) {
  // if string is constant, skip it, otherwise element was cached and should be destroyed
  if (!str.is_const_reference_counter()) {
    str.destroy_cached();
  }
  return true;
}

class AllocReplaceSection {
public:
  void lock() {
    dl::enter_critical_section();
    php_assert(!dl::replace_script_allocator_with_malloc && !dl::replace_malloc_with_script_allocator);
    dl::replace_script_allocator_with_malloc = true;
    mem_on_lock_ = dl::static_memory_used;
  }

  void unlock() {
    total_mem_allocated_ += (dl::static_memory_used - mem_on_lock_);
    php_assert(dl::replace_script_allocator_with_malloc && !dl::replace_malloc_with_script_allocator);
    dl::replace_script_allocator_with_malloc = false;
    dl::leave_critical_section();
  }

  int64_t total_mem_allocated() const {
    return dl::replace_script_allocator_with_malloc ?
           total_mem_allocated_ + dl::static_memory_used - mem_on_lock_ :
           total_mem_allocated_;
  }

private:
  int64_t total_mem_allocated_{0};
  int64_t mem_on_lock_{0};
};

class AllocForbiddenSection {
public:
  void lock() {
    dl::enter_critical_section();
    dl::forbid_malloc_allocations = true;
  }

  void unlock() {
    dl::forbid_malloc_allocations = false;
    dl::leave_critical_section();
  }
};

class InstanceCache {
private:
  struct Hash {
    size_t operator()(const string &str) const {
      return static_cast<size_t>(str.hash());
    }
  };

  struct ElementHolder {
    bool is_alive(std::chrono::seconds now) { return expiring_at > now; }

    std::chrono::seconds expiring_at{std::chrono::seconds::max()};
    std::unique_ptr<InstanceWrapperBase> instance_wrapper;
  };

  using Storage_ = std::unordered_map<string, ElementHolder, Hash>;
  using Garbage_ = std::forward_list<ElementHolder>;
  using ExpirationTrace_ = std::multimap<std::chrono::seconds, string>;

  InstanceCache() {
    refresh();
    std::lock_guard<AllocReplaceSection> lock(alloc_replace_);
    storage_ = vk::make_unique<Storage_>();
    garbage_ = vk::make_unique<Garbage_>();
    expiration_trace_ = vk::make_unique<ExpirationTrace_>();
  }

public:
  static InstanceCache &get() {
    static InstanceCache cache;
    return cache;
  }

  void refresh() {
    now_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch());
  }

  bool store(string key, const InstanceWrapperBase &instance_wrapper, int ttl) {
    std::lock_guard<AllocReplaceSection> lock(alloc_replace_);
    auto detached_instance = instance_wrapper.clone_and_detach_shared_ref();
    if (!detached_instance) {
      return false;
    }
    auto it = storage_->find(key);
    if (it == storage_->end()) {
      if (!DeepSharedDetach{}.process(key)) {
        detached_instance->memory_limit_warning();
        delete detached_instance;
        return false;
      }
      it = storage_->emplace(std::move(key), ElementHolder{}).first;
    } else {
      detach_expiration_trace(it);
      garbage_->emplace_front(std::move(it->second));
    }

    it->second.expiring_at = calc_expiring(ttl);
    it->second.instance_wrapper.reset(detached_instance);
    attach_expiration_trace(it);
    return true;
  }

  InstanceWrapperBase *fetch(const string &key) {
    std::lock_guard<AllocForbiddenSection> lock(alloc_forbidden_);
    auto it = storage_->find(key);
    if (it != storage_->end()) {
      php_assert(it->second.is_alive(now_));
      return it->second.instance_wrapper.get();
    }
    return nullptr;
  }

  bool del(const string &key) {
    auto it = storage_->find(key);
    if (it == storage_->end()) {
      return false;
    }
    php_assert(it->second.is_alive(now_));
    std::lock_guard<AllocReplaceSection> lock(alloc_replace_);
    move_to_garbage(it);
    return true;
  }

  void clear() {
    std::lock_guard<AllocReplaceSection> lock(alloc_replace_);
    for (auto it = storage_->begin(); it != storage_->end();) {
      it = move_to_garbage(it);
    }
  }

  void purge_expired() {
    std::lock_guard<AllocReplaceSection> alloc_replace_section_lock(alloc_replace_);
    std::lock_guard<AllocForbiddenSection> alloc_forbidden_lock(alloc_forbidden_);
    for (auto it = expiration_trace_->begin(); it != expiration_trace_->end() && it->first <= now_;) {
      auto storage_it = storage_->find(it->second);
      php_assert(storage_it != storage_->end());
      php_assert(storage_it->second.expiring_at == it->first);

      storage_->erase(storage_it);
      DeepDestroy{}.process(it->second);
      it = expiration_trace_->erase(it);
    }
  }

  void purge_garbage() {
    std::lock_guard<AllocReplaceSection> alloc_replace_section_lock(alloc_replace_);
    std::lock_guard<AllocForbiddenSection> alloc_forbidden_lock(alloc_forbidden_);
    garbage_->clear();
  }

  bool is_memory_limit_exceeded() const {
    return alloc_replace_.total_mem_allocated() >= total_memory_limit_;
  }

  void set_memory_limit(int64_t limit) {
    total_memory_limit_ = limit;
  }

  ~InstanceCache() {
    clear();
    purge_garbage();

    std::lock_guard<AllocReplaceSection> alloc_replace_section_lock(alloc_replace_);
    std::lock_guard<AllocForbiddenSection> alloc_forbidden_lock(alloc_forbidden_);
    php_assert(storage_->empty());
    storage_.reset();
    php_assert(garbage_->empty());
    garbage_.reset();
    php_assert(expiration_trace_->empty());
    expiration_trace_.reset();
  }

  void print_debug_report() const {
    fprintf(stderr, "-----------Debug report-----------\n");
    fprintf(stderr, "  timestamp now: %ld\n", now_.count());
    fprintf(stderr, "  elements in cache: %zu\n", storage_->size());
    fprintf(stderr, "  elements in expiration trace: %zu\n", expiration_trace_->size());
    fprintf(stderr, "  total memory allocated: %ld\n", alloc_replace_.total_mem_allocated());
    fprintf(stderr, "  total memory limit: %ld\n", total_memory_limit_);
    fprintf(stderr, "  garbage is empty: %s\n", garbage_->empty() ? "true" : "false");
    fprintf(stderr, "----------------------------------\n");
  }

private:
  void detach_expiration_trace(Storage_::iterator it) {
    auto range = expiration_trace_->equal_range(it->second.expiring_at);
    auto trace_it = std::find_if(range.first, range.second,
                                 [it](const ExpirationTrace_::value_type &kv) {
                                   return kv.second == it->first;
                                 });
    if (trace_it != range.second) {
      expiration_trace_->erase(trace_it);
    }
  }

  void attach_expiration_trace(Storage_::iterator it) {
    if (it->second.expiring_at != std::chrono::seconds::max()) {
      expiration_trace_->emplace_hint(expiration_trace_->end(), it->second.expiring_at, it->first);
    }
  }

  Storage_::iterator move_to_garbage(Storage_::iterator it) {
    garbage_->emplace_front(std::move(it->second));

    std::lock_guard<AllocForbiddenSection> lock(alloc_forbidden_);
    detach_expiration_trace(it);
    string removing_key = it->first;
    auto next_it = storage_->erase(it);
    DeepDestroy{}.process(removing_key);
    return next_it;
  }

  std::chrono::seconds calc_expiring(int ttl) const {
    return ttl > 0 ? now_ + std::chrono::seconds{ttl} : std::chrono::seconds::max();
  }

  int64_t total_memory_limit_{16 * 1024 * 1024};
  std::chrono::seconds now_{std::chrono::seconds::zero()};
  AllocForbiddenSection alloc_forbidden_;
  AllocReplaceSection alloc_replace_;
  std::unique_ptr<Storage_> storage_;
  std::unique_ptr<Garbage_> garbage_;
  std::unique_ptr<ExpirationTrace_> expiration_trace_;
};

bool is_instance_cache_memory_limit_exceeded_() {
  return InstanceCache::get().is_memory_limit_exceeded();
}

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int ttl) {
  return InstanceCache::get().store(key, instance_wrapper, ttl);
}

InstanceWrapperBase *instance_cache_fetch_wrapper(const string &key) {
  return InstanceCache::get().fetch(key);
}
} // namespace ic_impl_

void init_instance_cache_lib() {
  // Before each request we refresh current time point and remove expired elements
  // Time point refreshes only once for request
  ic_impl_::InstanceCache::get().refresh();
  ic_impl_::InstanceCache::get().purge_expired();
}

void free_instance_cache_lib() {
  // After each request we fully remove implicitly deleted elements
  ic_impl_::InstanceCache::get().purge_garbage();
}

void set_instance_cache_memory_limit(int64_t limit) {
  ic_impl_::InstanceCache::get().set_memory_limit(limit);
}

bool f$instance_cache_delete(const string &key) {
  return ic_impl_::InstanceCache::get().del(key);
}

bool f$instance_cache_clear() {
  ic_impl_::InstanceCache::get().clear();
  return true;
}

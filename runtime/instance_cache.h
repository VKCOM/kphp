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

#include "runtime/kphp_core.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"
#include "runtime/shape.h"

namespace ic_impl_ {

template<typename Child>
class BasicVisitor : vk::not_copyable {
public:
  template<typename T>
  void operator()(const char *, T &&value) {
    is_ok_ = child_.process(std::forward<T>(value));
  }

  template<typename T>
  bool process(T &) { return true; }

  template<typename T>
  bool process(Optional<T> &value) {
    return value.has_value() ? child_.process(value.val()) : true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) {
    static_assert(!std::is_abstract<I>::value, "instance_cache doesn't support interfaces");
    if (!instance.is_null()) {
      instance->accept(child_);
      return child_.is_ok();
    }
    return true;
  }

  template<typename ...Args>
  bool process(std::tuple<Args...> &value) {
    return process_tuple(value);
  }

  template<size_t ...Is, typename ...T>
  bool process(shape<std::index_sequence<Is...>, T...> &value) {
    const bool child_res[] = {child_.process(value.template get<Is>())...};
    return std::all_of(std::begin(child_res), std::end(child_res), [](bool r) { return r; });
  }

  bool process(mixed &value) {
    if (value.is_string()) {
      return child_.process(value.as_string());
    } else if (value.is_array()) {
      return child_.process(value.as_array());
    }
    return true;
  }

  bool is_ok() const { return is_ok_; }

protected:
  explicit BasicVisitor(Child &child) :
    child_(child) {
  }

  template<typename Iterator>
  bool process_range(Iterator first, Iterator last) {
    bool res = true;
    for (; first != last; ++first) {
      if (!child_.process(first.get_value())) {
        res = false;
      }
      if (first.is_string_key() && !child_.process(first.get_string_key())) {
        res = false;
      }
    }
    return res;
  }

private:
  template<size_t Index = 0, typename ...Args>
  std::enable_if_t<Index != sizeof...(Args), bool> process_tuple(std::tuple<Args...> &value) {
    bool res = child_.process(std::get<Index>(value));
    return process_tuple<Index + 1>(value) && res;
  }

  template<size_t Index = 0, typename ...Args>
  std::enable_if_t<Index == sizeof...(Args), bool> process_tuple(std::tuple<Args...> &) {
    return true;
  }

  bool is_ok_{true};
  Child &child_;
};

class DeepMoveFromScriptToCacheVisitor : BasicVisitor<DeepMoveFromScriptToCacheVisitor> {
public:
  using Basic = BasicVisitor<DeepMoveFromScriptToCacheVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  explicit DeepMoveFromScriptToCacheVisitor(memory_resource::unsynchronized_pool_resource &memory_pool) noexcept;

  template<typename T>
  bool process(array<T> &arr) {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }
    if (unlikely(!is_enough_memory_for(arr.estimate_memory_usage()))) {
      arr = array<T>();
      memory_limit_exceeded_ = true;
      return false;
    }
    // begin mutates array implicitly, therefore call it separately
    auto first = arr.begin();
    // mutates may make array as constant again (e.g. empty array), therefore check again
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      php_assert(first == arr.end());
      return true;
    }
    php_assert(arr.get_reference_counter() == 1);
    arr.set_reference_counter_to(ExtraRefCnt::for_instance_cache);
    return Basic::process_range(first, arr.end());
  }

  template<typename I>
  bool process(class_instance<I> &instance) {
    if (++instance_depth_level_ >= instance_depth_level_limit_) {
      instance.destroy();
      is_depth_limit_exceeded_ = true;
      return false;
    }

    bool result = true;
    if (!instance.is_null()) {
      if (likely(is_enough_memory_for(instance.estimate_memory_usage()))) {
        instance = instance.clone();
        instance.set_reference_counter_to_cache();
        result = Basic::process(instance);
      } else {
        instance.destroy();
        result = false;
        memory_limit_exceeded_ = true;
      }
    }
    --instance_depth_level_;
    return result;
  }

  bool process(string &str);

  bool is_depth_limit_exceeded() const {
    return is_depth_limit_exceeded_;
  }

  bool is_memory_limit_exceeded() const {
    return memory_limit_exceeded_;
  }

  bool is_ok() const noexcept {
    return !is_depth_limit_exceeded() && !is_memory_limit_exceeded();
  }

  bool is_enough_memory_for(size_t size) noexcept {
    if (!memory_limit_exceeded_) {
      memory_limit_exceeded_ = !memory_pool_.is_enough_memory_for(size);
    }
    return !memory_limit_exceeded_;
  }

  void *prepare_raw_memory(size_t size) noexcept {
    return is_enough_memory_for(size) ? memory_pool_.allocate(size) : nullptr;
  }

private:
  bool memory_limit_exceeded_{false};
  bool is_depth_limit_exceeded_{false};
  uint8_t instance_depth_level_{0u};
  const uint8_t instance_depth_level_limit_{128u};
  memory_resource::unsynchronized_pool_resource &memory_pool_;
};

class DeepDestroyFromCacheVisitor : BasicVisitor<DeepDestroyFromCacheVisitor> {
public:
  using Basic = BasicVisitor<DeepDestroyFromCacheVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  DeepDestroyFromCacheVisitor();

  template<typename T>
  bool process(array<T> &arr) {
    // if array is constant, skip it, otherwise element was cached and should be destroyed
    if (!arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
      arr.force_destroy(ExtraRefCnt::for_instance_cache);
    }
    return true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) {
    Basic::process(instance);
    instance.destroy_cached();
    return true;
  }

  bool process(string &str);
};

class InstanceWrapperBase : public ManagedThroughDlAllocator, vk::not_copyable {
public:
  virtual const char *get_class() const noexcept = 0;
  virtual std::unique_ptr<InstanceWrapperBase> clone_and_detach_shared_ref(
    DeepMoveFromScriptToCacheVisitor &detach_processor) const noexcept = 0;
  virtual std::unique_ptr<InstanceWrapperBase> clone_on_script_memory() const noexcept = 0;
  virtual ~InstanceWrapperBase() noexcept = default;
};

template<typename I>
class InstanceWrapper;

template<typename I>
class InstanceWrapper<class_instance<I>> final : public InstanceWrapperBase {
public:
  explicit InstanceWrapper(const class_instance<I> &instance, bool deep_destroy_required = false) noexcept:
    instance_(instance),
    deep_destroy_required_(deep_destroy_required) {
  }

  const char *get_class() const noexcept final {
    return instance_.get_class();
  }

  std::unique_ptr<InstanceWrapperBase> clone_and_detach_shared_ref(
    DeepMoveFromScriptToCacheVisitor &detach_processor) const noexcept final {
    auto detached_instance = instance_;
    detach_processor.process(detached_instance);

    // sizeof(size_t) - an extra memory that we need inside the make_unique_on_script_memory
    constexpr auto size_for_wrapper = sizeof(size_t) + sizeof(InstanceWrapper<class_instance<I>>);
    if (unlikely(detach_processor.is_depth_limit_exceeded() ||
                 !detach_processor.is_enough_memory_for(size_for_wrapper))) {
      DeepDestroyFromCacheVisitor{}.process(detached_instance);
      return {};
    }
    return make_unique_on_script_memory<InstanceWrapper<class_instance<I>>>(detached_instance, true);
  }

  std::unique_ptr<InstanceWrapperBase> clone_on_script_memory() const noexcept final {
    return make_unique_on_script_memory<InstanceWrapper<class_instance<I>>>(instance_);
  }

  class_instance<I> get_instance() const noexcept {
    return instance_;
  }

  ~InstanceWrapper() noexcept final {
    if (deep_destroy_required_ && !instance_.is_null()) {
      DeepDestroyFromCacheVisitor{}.process(instance_);
    }
  }

private:
  class_instance<I> instance_;
  const bool deep_destroy_required_{false};
};

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int64_t ttl);
const InstanceWrapperBase *instance_cache_fetch_wrapper(const string &key, bool even_if_expired);

} // namespace ic_impl_

using DeepMoveFromScriptToCacheVisitor = ic_impl_::DeepMoveFromScriptToCacheVisitor;
using DeepDestroyFromCacheVisitor = ic_impl_::DeepDestroyFromCacheVisitor;

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
bool f$instance_cache_store(const string &key, const ClassInstanceType &instance, int64_t ttl = 0) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (instance.is_null()) {
    return false;
  }
  ic_impl_::InstanceWrapper<ClassInstanceType> instance_wrapper{instance};
  return ic_impl_::instance_cache_store(key, instance_wrapper, ttl);
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string &class_name, const string &key, bool even_if_expired = false) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (const auto *base_wrapper = ic_impl_::instance_cache_fetch_wrapper(key, even_if_expired)) {
    // do not use first parameter (class name) for verifying type,
    // because different classes from separated libs may have same names
    if (auto wrapper = dynamic_cast<const ic_impl_::InstanceWrapper<ClassInstanceType> *>(base_wrapper)) {
      auto result = wrapper->get_instance();
      php_assert(!result.is_null());
      return result;
    } else {
      php_warning("Trying to fetch incompatible instance class: expect '%s', got '%s'",
                  class_name.c_str(), base_wrapper->get_class());
    }
  }
  return {};
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch_immutable(const string &class_name, const string &key, bool even_if_expired = false) {
  return f$instance_cache_fetch<ClassInstanceType>(class_name, key, even_if_expired);
}

bool f$instance_cache_update_ttl(const string &key, int64_t ttl = 0);
bool f$instance_cache_delete(const string &key);

#pragma once

// This API inspired by APC and allow to cache any kind of instances between requests.
// Highlights:
//  1) All strings, arrays, instances are placed into common heap memory;
//  2) On store, all constant strings and arrays (check REF_CNT_FOR_CONST) are shallow copied as is,
//    otherwise, a deep copy is used and the reference counter is set to special value (check REF_CNT_FOR_CACHE),
//    therefore the reference counter of cached strings and arrays is REF_CNT_FOR_CACHE or REF_CNT_FOR_CONST;
//  3) On fetch, all strings and arrays are returned as is;
//  4) On store, all instances (and sub instances) are deeply cloned into instance cache;
//  5) On fetch, all instances (and sub instances) are deeply cloned from instance cache;
//  6) All instances (with all members) are destroyed strictly before or after request,
//    and shouldn't be destroyed while request.

#include "runtime/kphp_core.h"
#include "common/mixin/not_copyable.h"
#include "common/type_traits/traits.h"

namespace ic_impl_ {

bool is_instance_cache_memory_limit_exceeded_();

template<typename Child>
class BasicVisitor {
public:
  template<typename T>
  void operator()(const char *, T &&value) {
    is_ok_ = child_.process(std::forward<T>(value));
  }

  template<typename T>
  bool process(T &) { return true; }

  template<typename T>
  bool process(OrFalse<T> &value) {
    return value.bool_value ? child_.process(value.val()) : true;
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

  bool process(var &value) {
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
  vk::enable_if_t<Index != sizeof...(Args), bool> process_tuple(std::tuple<Args...> &value) {
    bool res = child_.process(std::get<Index>(value));
    return process_tuple<Index + 1>(value) && res;
  }

  template<size_t Index = 0, typename ...Args>
  vk::enable_if_t<Index == sizeof...(Args), bool> process_tuple(std::tuple<Args...> &) {
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

  DeepMoveFromScriptToCacheVisitor();

  template<typename T>
  bool process(array<T> &arr) {
    if (is_memory_limit_exceeded()) {
      arr = array<T>();
      return false;
    }
    bool result = true;
    if (!arr.is_const_reference_counter()) {
      // begin mutates array implicitly, therefore call it separately
      auto first = arr.begin();
      result = Basic::process_range(first, arr.end());
    }

    // mutates may make array as constant again (e.g. empty array), therefore check again
    if (!arr.is_const_reference_counter()) {
      arr.set_reference_counter_to_cache();
    }
    return result;
  }

  template<typename I>
  bool process(class_instance<I> &instance) {
    if (is_memory_limit_exceeded()) {
      instance.destroy();
      return false;
    }
    if (++instance_depth_level_ < instance_depth_level_limit_) {
      if (!instance.is_null()) {
        instance = instance.clone();
        instance.set_reference_counter_to_cache();
        Basic::process(instance);
      }
      --instance_depth_level_;
      return true;
    } else {
      instance.destroy();
      is_depth_limit_exceeded_ = true;
      return false;
    }
  }

  bool process(string &str);

  bool is_depth_limit_exceeded() const {
    return is_depth_limit_exceeded_;
  }

  bool is_memory_limit_exceeded() {
    if (!is_memory_limit_exceeded_) {
      is_memory_limit_exceeded_ = is_instance_cache_memory_limit_exceeded_();
    }
    return is_memory_limit_exceeded_;
  }

private:
  bool is_memory_limit_exceeded_{false};
  bool is_depth_limit_exceeded_{false};
  uint8_t instance_depth_level_{0u};
  const uint8_t instance_depth_level_limit_{128u};
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
    if (!arr.is_const_reference_counter()) {
      Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
      arr.destroy_cached();
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

class ShallowMoveFromCacheToScriptVisitor : BasicVisitor<ShallowMoveFromCacheToScriptVisitor> {
public:
  using Basic = BasicVisitor<ShallowMoveFromCacheToScriptVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  ShallowMoveFromCacheToScriptVisitor();

  template<typename I>
  bool process(class_instance<I> &instance) {
    instance = instance.clone();
    return Basic::process(instance);
  }

  bool process(var &) { return true; }

  template<typename T>
  vk::enable_if_t<is_class_instance_inside<T>::value, bool> process(array<T> &arr) {
    // begin mutates array implicitly, therefore call it separately
    auto first = arr.begin();
    return Basic::process_range(first, arr.end());
  }
};

class InstanceWrapperBase : vk::not_copyable {
public:
  virtual const char *get_class() const = 0;
  virtual InstanceWrapperBase *clone_and_detach_shared_ref() const = 0;
  virtual void memory_limit_warning() const = 0;
  virtual ~InstanceWrapperBase() = default;
};

template<typename I>
class InstanceWrapper;

template<typename I>
class InstanceWrapper<class_instance<I>> final : public InstanceWrapperBase {
public:
  explicit InstanceWrapper(const class_instance<I> &instance) :
    instance_(instance) {
  }

  const char *get_class() const final {
    return instance_.get_class();
  }

  void memory_limit_warning() const final {
    php_warning("Memory limit exceeded on cloning instance of class '%s' into cache", get_class());
  }

  InstanceWrapperBase *clone_and_detach_shared_ref() const final {
    auto detached_instance = instance_;
    DeepMoveFromScriptToCacheVisitor detach_processor;
    detach_processor.process(detached_instance);
    if (detach_processor.is_memory_limit_exceeded()) {
      memory_limit_warning();
      DeepDestroyFromCacheVisitor{}.process(detached_instance);
      return nullptr;
    }
    if (detach_processor.is_depth_limit_exceeded()) {
      php_warning("Depth limit exceeded on cloning instance of class '%s' into cache", get_class());
      DeepDestroyFromCacheVisitor{}.process(detached_instance);
      return nullptr;
    }
    return new InstanceWrapper<class_instance<I>>{detached_instance};
  }

  class_instance<I> deep_instance_clone() const {
    auto cloned_instance = instance_;
    ShallowMoveFromCacheToScriptVisitor{}.process(cloned_instance);
    return cloned_instance;
  }

  class_instance<I> get_instance() const {
    return instance_;
  }

  void clear() {
    instance_.destroy();
  }

  ~InstanceWrapper() final {
    if (!instance_.is_null()) {
      DeepDestroyFromCacheVisitor{}.process(instance_);
    }
  }

private:
  class_instance<I> instance_;
};

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int ttl);
InstanceWrapperBase *instance_cache_fetch_wrapper(const string &key);

template<typename ClassInstanceType>
ClassInstanceType instance_cache_fetch(const string &class_name, const string &key, bool deep_clone) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (auto base_wrapper = ic_impl_::instance_cache_fetch_wrapper(key)) {
    // do not use first parameter (class name) for verifying type,
    // because different classes from separated libs may have same names
    if (auto wrapper = dynamic_cast<ic_impl_::InstanceWrapper<ClassInstanceType> *>(base_wrapper)) {
      auto result = deep_clone ?
                    wrapper->deep_instance_clone() :
                    wrapper->get_instance();
      php_assert(!result.is_null());
      return result;
    } else {
      php_warning("Trying to fetch incompatible instance class: expect '%s', got '%s'",
                  class_name.c_str(), base_wrapper->get_class());
    }
  }
  return false;
}

} // namespace ic_impl_

using DeepMoveFromScriptToCacheVisitor = ic_impl_::DeepMoveFromScriptToCacheVisitor;
using DeepDestroyFromCacheVisitor = ic_impl_::DeepDestroyFromCacheVisitor;
using ShallowMoveFromCacheToScriptVisitor = ic_impl_::ShallowMoveFromCacheToScriptVisitor;

void init_instance_cache_lib();
void free_instance_cache_lib();
void set_instance_cache_memory_limit(int64_t limit);

template<typename ClassInstanceType>
bool f$instance_cache_store(const string &key, const ClassInstanceType &instance, int ttl = 0) {
  static_assert(is_class_instance<ClassInstanceType>::value, "class_instance<> type expected");
  if (instance.is_null()) {
    return false;
  }
  ic_impl_::InstanceWrapper<ClassInstanceType> instance_wrapper{instance};
  const bool result = ic_impl_::instance_cache_store(key, instance_wrapper, ttl);
  instance_wrapper.clear();
  return result;
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string &class_name, const string &key) {
  return ic_impl_::instance_cache_fetch<ClassInstanceType>(class_name, key, true);
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch_immutable(const string &class_name, const string &key) {
  return ic_impl_::instance_cache_fetch<ClassInstanceType>(class_name, key, false);
}

bool f$instance_cache_delete(const string &key);
bool f$instance_cache_clear();

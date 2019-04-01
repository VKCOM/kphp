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

#include "common/mixin/not_copyable.h"
#include "common/type_traits/traits.h"
#include "runtime/kphp_core.h"

namespace ic_impl_ {

bool is_instance_cache_memory_limit_exceeded_();

template<typename I, typename ProcessorImpl>
class InstanceVisitor {
public:
  explicit InstanceVisitor(class_instance<I> &instance, ProcessorImpl &processor) :
    instance_(instance),
    processor_(processor) {
  }

  template<typename T>
  void operator()(T I::* value) {
    // we need to process each element
    if (!processor_.process(instance_.get()->*value)) {
      is_ok_ = false;
    }
  }

  bool is_ok() const { return is_ok_; }

private:
  bool is_ok_{true};
  class_instance<I> &instance_;
  ProcessorImpl &processor_;
};

template<typename Child>
class BasicProcessor {
public:
  template<typename T>
  bool process(T &) { return true; }

  template<typename T>
  bool process(OrFalse<T> &value) {
    return value.bool_value ? child_.process(value.val()) : true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) {
    if (!instance.is_null()) {
      InstanceVisitor<I, Child> visitor{instance, child_};
      instance->accept(visitor);
      return visitor.is_ok();
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

protected:
  explicit BasicProcessor(Child &child) :
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

  Child &child_;
};

class DeepSharedDetach : BasicProcessor<DeepSharedDetach> {
public:
  using Basic = BasicProcessor<DeepSharedDetach>;
  using Basic::process;

  DeepSharedDetach();

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
      instance = instance.clone();
      Basic::process(instance);
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

class DeepDestroy : BasicProcessor<DeepDestroy> {
public:
  using Basic = BasicProcessor<DeepDestroy>;
  using Basic::process;

  DeepDestroy();

  template<typename T>
  bool process(array<T> &arr) {
    // if array is constant, skip it, otherwise element was cached and should be destroyed
    if (!arr.is_const_reference_counter()) {
      Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
      arr.destroy_cached();
    }
    return true;
  }

  bool process(string &str);
};

class DeepInstanceClone : BasicProcessor<DeepInstanceClone> {
public:
  using Basic = BasicProcessor<DeepInstanceClone>;
  using Basic::process;

  DeepInstanceClone();

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
class InstanceWrapper final : public InstanceWrapperBase {
private:
  static_assert(is_class_instance<I>::value, "class_instance<> type expected");
  using ClassType = typename I::ClassType;

public:
  explicit InstanceWrapper(const class_instance<ClassType> &instance) :
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
    DeepSharedDetach detach_processor;
    detach_processor.process(detached_instance);
    if (detach_processor.is_memory_limit_exceeded()) {
      memory_limit_warning();
      DeepDestroy{}.process(detached_instance);
      return nullptr;
    }
    if (detach_processor.is_depth_limit_exceeded()) {
      php_warning("Depth limit exceeded on cloning instance of class '%s' into cache", get_class());
      DeepDestroy{}.process(detached_instance);
      return nullptr;
    }
    return new InstanceWrapper<class_instance<ClassType>>{detached_instance};
  }

  class_instance<ClassType> deep_instance_clone() const {
    php_assert(instance_.get_reference_counter() == 1);
    auto cloned_instance = instance_;
    DeepInstanceClone{}.process(cloned_instance);
    return cloned_instance;
  }

  void clear() {
    instance_.destroy();
  }

  ~InstanceWrapper() final {
    if (!instance_.is_null()) {
      DeepDestroy{}.process(instance_);
    }
  }

private:
  class_instance<ClassType> instance_;
};

bool instance_cache_store(const string &key, const InstanceWrapperBase &instance_wrapper, int ttl);
InstanceWrapperBase *instance_cache_fetch(const string &key);
} // namespace ic_impl_

void init_instance_cache_lib();
void free_instance_cache_lib();
extern "C" {
  void set_instance_cache_memory_limit(int64_t limit);
}

template<typename I>
bool f$instance_cache_store(const string &key, const I &instance, int ttl = 0) {
  static_assert(is_class_instance<I>::value, "class_instance<> type expected");
  static_assert(!std::is_abstract<typename I::ClassType>::value, "instance_cache_store doesn't support interfaces");
  if (instance.is_null()) {
    return false;
  }
  ic_impl_::InstanceWrapper<I> instance_wrapper{instance};
  const bool result = ic_impl_::instance_cache_store(key, instance_wrapper, ttl);
  instance_wrapper.clear();
  return result;
}

template<typename I>
I f$instance_cache_fetch(const string &class_name, const string &key) {
  static_assert(is_class_instance<I>::value, "class_instance<> type expected");
  if (auto base_wrapper = ic_impl_::instance_cache_fetch(key)) {
    // do not use first parameter (class name) for verifying type,
    // because different classes from separated libs may have same names
    if (auto instance_wrapper = dynamic_cast<ic_impl_::InstanceWrapper<I> *>(base_wrapper)) {
      auto result = instance_wrapper->deep_instance_clone();
      php_assert(!result.is_null());
      return result;
    } else {
      php_warning("Trying to fetch incompatible instance class: expect '%s', got '%s'",
                  class_name.c_str(), base_wrapper->get_class());
    }
  }
  return false;
}

bool f$instance_cache_delete(const string &key);
bool f$instance_cache_clear();

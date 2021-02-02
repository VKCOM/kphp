// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"

#include "runtime/kphp_core.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

namespace impl_ {

template<typename Child>
class InstanceDeepBasicVisitor : vk::not_copyable {
public:
  template<typename T>
  void operator()(const char *, T &&value) noexcept {
    is_ok_ = child_.process(std::forward<T>(value));
  }

  template<typename T>
  bool process(T &) noexcept { return true; }

  template<typename T>
  bool process(Optional<T> &value) noexcept {
    return value.has_value() ? child_.process(value.val()) : true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) noexcept {
    static_assert(!std::is_abstract<I>::value, "instance_cache doesn't support interfaces");
    if (!instance.is_null()) {
      instance->accept(child_);
      return child_.is_ok();
    }
    return true;
  }

  template<typename ...Args>
  bool process(std::tuple<Args...> &value) noexcept {
    return process_tuple(value);
  }

  template<size_t ...Is, typename ...T>
  bool process(shape<std::index_sequence<Is...>, T...> &value) noexcept {
    const bool child_res[] = {child_.process(value.template get<Is>())...};
    return std::all_of(std::begin(child_res), std::end(child_res), [](bool r) { return r; });
  }

  bool process(mixed &value) noexcept {
    if (value.is_string()) {
      return child_.process(value.as_string());
    } else if (value.is_array()) {
      return child_.process(value.as_array());
    }
    return true;
  }

  bool is_ok() const noexcept { return is_ok_; }

protected:
  explicit InstanceDeepBasicVisitor(Child &child) noexcept:
    child_(child) {
  }

  template<typename Iterator>
  bool process_range(Iterator first, Iterator last) noexcept {
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
  std::enable_if_t<Index != sizeof...(Args), bool> process_tuple(std::tuple<Args...> &value) noexcept {
    bool res = child_.process(std::get<Index>(value));
    return process_tuple<Index + 1>(value) && res;
  }

  template<size_t Index = 0, typename ...Args>
  std::enable_if_t<Index == sizeof...(Args), bool> process_tuple(std::tuple<Args...> &) noexcept {
    return true;
  }

  bool is_ok_{true};
  Child &child_;
};

} // namespace impl_

class InstanceDeepCopyVisitor : impl_::InstanceDeepBasicVisitor<InstanceDeepCopyVisitor> {
public:
  using Basic = InstanceDeepBasicVisitor<InstanceDeepCopyVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  InstanceDeepCopyVisitor(memory_resource::unsynchronized_pool_resource &memory_pool, ExtraRefCnt memory_ref_cnt) noexcept;

  template<typename T>
  bool process(array<T> &arr) noexcept {
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
    arr.set_reference_counter_to(memory_ref_cnt_);
    return Basic::process_range(first, arr.end());
  }

  template<typename I>
  bool process(class_instance<I> &instance) noexcept {
    if (++instance_depth_level_ >= instance_depth_level_limit_) {
      instance.destroy();
      is_depth_limit_exceeded_ = true;
      return false;
    }

    bool result = true;
    if (!instance.is_null()) {
      if (likely(is_enough_memory_for(instance.estimate_memory_usage()))) {
        instance = instance.clone();
        instance.set_reference_counter_to(memory_ref_cnt_);
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

  bool process(string &str) noexcept;

  bool is_depth_limit_exceeded() const noexcept {
    return is_depth_limit_exceeded_;
  }

  bool is_memory_limit_exceeded() const noexcept {
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
  const ExtraRefCnt memory_ref_cnt_{ExtraRefCnt::for_global_const};
  memory_resource::unsynchronized_pool_resource &memory_pool_;
};

class InstanceDeepDestroyVisitor : impl_::InstanceDeepBasicVisitor<InstanceDeepDestroyVisitor> {
public:
  using Basic = InstanceDeepBasicVisitor<InstanceDeepDestroyVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  explicit InstanceDeepDestroyVisitor(ExtraRefCnt memory_ref_cnt) noexcept;

  template<typename T>
  bool process(array<T> &arr) noexcept{
    // if array is constant, skip it, otherwise element was cached and should be destroyed
    if (!arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
      arr.force_destroy(memory_ref_cnt_);
    }
    return true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) noexcept{
    Basic::process(instance);
    instance.force_destroy(memory_ref_cnt_);
    return true;
  }

  bool process(string &str) noexcept;

private:
  const ExtraRefCnt memory_ref_cnt_{ExtraRefCnt::for_global_const};
};

class InstanceCopyistBase : public ManagedThroughDlAllocator, vk::not_copyable {
public:
  virtual const char *get_class() const noexcept = 0;
  virtual std::unique_ptr<InstanceCopyistBase> clone_and_detach_shared_ref(InstanceDeepCopyVisitor &detach_processor) const noexcept = 0;
  virtual std::unique_ptr<InstanceCopyistBase> clone_on_script_memory() const noexcept = 0;
  virtual ~InstanceCopyistBase() noexcept = default;
};

template<typename I>
class InstanceCopyistImpl;

template<typename I>
class InstanceCopyistImpl<class_instance<I>> final : public InstanceCopyistBase {
public:
  explicit InstanceCopyistImpl(const class_instance<I> &instance, bool deep_destroy_required = false) noexcept:
    instance_(instance),
    deep_destroy_required_(deep_destroy_required) {
  }

  const char *get_class() const noexcept final {
    return instance_.get_class();
  }

  std::unique_ptr<InstanceCopyistBase> clone_and_detach_shared_ref(InstanceDeepCopyVisitor &detach_processor) const noexcept final {
    auto detached_instance = instance_;
    detach_processor.process(detached_instance);

    // sizeof(size_t) - an extra memory that we need inside the make_unique_on_script_memory
    constexpr auto size_for_wrapper = sizeof(size_t) + sizeof(InstanceCopyistImpl<class_instance<I>>);
    if (unlikely(detach_processor.is_depth_limit_exceeded() ||
                 !detach_processor.is_enough_memory_for(size_for_wrapper))) {
      InstanceDeepDestroyVisitor{ExtraRefCnt::for_instance_cache}.process(detached_instance);
      return {};
    }
    return make_unique_on_script_memory<InstanceCopyistImpl<class_instance<I>>>(detached_instance, true);
  }

  std::unique_ptr<InstanceCopyistBase> clone_on_script_memory() const noexcept final {
    return make_unique_on_script_memory<InstanceCopyistImpl<class_instance<I>>>(instance_);
  }

  class_instance<I> get_instance() const noexcept {
    return instance_;
  }

  ~InstanceCopyistImpl() noexcept final {
    if (deep_destroy_required_ && !instance_.is_null()) {
      InstanceDeepDestroyVisitor{ExtraRefCnt::for_instance_cache}.process(instance_);
    }
  }

private:
  class_instance<I> instance_;
  const bool deep_destroy_required_{false};
};

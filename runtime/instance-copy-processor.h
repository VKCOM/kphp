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
    if (!instance.is_null()) {
      instance.get()->accept(child_);
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

  ExtraRefCnt get_memory_ref_cnt() const noexcept { return memory_ref_cnt_; }

protected:
  InstanceDeepBasicVisitor(Child &child, bool set_memory_ref_cnt, ExtraRefCnt memory_ref_cnt = ExtraRefCnt::for_global_const) noexcept:
    set_memory_ref_cnt_(set_memory_ref_cnt),
    memory_ref_cnt_(memory_ref_cnt),
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

  bool set_memory_ref_cnt_{false};

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
  const ExtraRefCnt memory_ref_cnt_{ExtraRefCnt::for_global_const};
  Child &child_;
};

} // namespace impl_

class InstanceDeepCopyVisitor : impl_::InstanceDeepBasicVisitor<InstanceDeepCopyVisitor> {
public:
  using Basic = InstanceDeepBasicVisitor<InstanceDeepCopyVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;
  using Basic::get_memory_ref_cnt;

  explicit InstanceDeepCopyVisitor(memory_resource::unsynchronized_pool_resource &memory_pool) noexcept;
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
    if (set_memory_ref_cnt_) {
      arr.set_reference_counter_to(get_memory_ref_cnt());
    }
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
        instance = instance.virtual_builtin_clone();
        if (set_memory_ref_cnt_) {
          instance.set_reference_counter_to(get_memory_ref_cnt());
        }
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
      arr.force_destroy(get_memory_ref_cnt());
    }
    return true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) noexcept{
    Basic::process(instance);
    instance.force_destroy(get_memory_ref_cnt());
    return true;
  }

  bool process(string &str) noexcept;
};

class InstanceCopyistBase : public ManagedThroughDlAllocator, vk::not_copyable {
public:
  virtual const char *get_class() const noexcept = 0;
  virtual std::unique_ptr<InstanceCopyistBase> deep_copy_and_set_ref_cnt(InstanceDeepCopyVisitor &detach_processor) const noexcept = 0;
  virtual std::unique_ptr<InstanceCopyistBase> shallow_copy() const noexcept = 0;
  virtual ~InstanceCopyistBase() noexcept = default;
};

template<typename I>
class InstanceCopyistImpl;

template<typename I>
class InstanceCopyistImpl<class_instance<I>> final : public InstanceCopyistBase {
public:
  explicit InstanceCopyistImpl(const class_instance<I> &instance) noexcept:
    instance_(instance) {
  }

  InstanceCopyistImpl(class_instance<I> &&instance, ExtraRefCnt memory_ref_cnt) noexcept:
    instance_(std::move(instance)),
    memory_ref_cnt_(memory_ref_cnt) {
  }

  const char *get_class() const noexcept final {
    return instance_.get_class();
  }

  std::unique_ptr<InstanceCopyistBase> deep_copy_and_set_ref_cnt(InstanceDeepCopyVisitor &detach_processor) const noexcept final {
    auto detached_instance = instance_;
    detach_processor.process(detached_instance);

    const auto memory_ref_cnt = detach_processor.get_memory_ref_cnt();

    // sizeof(size_t) - an extra memory that we need inside the make_unique_on_script_memory
    constexpr auto size_for_wrapper = sizeof(size_t) + sizeof(InstanceCopyistImpl<class_instance<I>>);
    if (unlikely(detach_processor.is_depth_limit_exceeded() ||
                 !detach_processor.is_enough_memory_for(size_for_wrapper))) {
      InstanceDeepDestroyVisitor{memory_ref_cnt}.process(detached_instance);
      return {};
    }
    return make_unique_on_script_memory<InstanceCopyistImpl<class_instance<I>>>(std::move(detached_instance), memory_ref_cnt);
  }

  std::unique_ptr<InstanceCopyistBase> shallow_copy() const noexcept final {
    return make_unique_on_script_memory<InstanceCopyistImpl<class_instance<I>>>(instance_);
  }

  class_instance<I> get_instance() const noexcept {
    return instance_;
  }

  ~InstanceCopyistImpl() noexcept final {
    if (memory_ref_cnt_ && !instance_.is_null()) {
      InstanceDeepDestroyVisitor{static_cast<ExtraRefCnt::extra_ref_cnt_value>(memory_ref_cnt_)}.process(instance_);
    }
  }

private:
  class_instance<I> instance_;
  const int memory_ref_cnt_{0};
};

template<class T>
class_instance<T> copy_instance_into_other_memory(const class_instance<T> &instance,
                                                  memory_resource::unsynchronized_pool_resource &memory_pool,
                                                  ExtraRefCnt memory_ref_cnt,
                                                  // TODO this param is used only for x2, so it can be removed with x2
                                                  bool force_enable_allocator = false) noexcept {
  dl::set_current_script_allocator(memory_pool, force_enable_allocator);

  class_instance<T> copied_instance = instance;
  InstanceDeepCopyVisitor copyVisitor{memory_pool, memory_ref_cnt};
  copyVisitor.process(copied_instance);
  if (unlikely(copyVisitor.is_depth_limit_exceeded())) {
    InstanceDeepDestroyVisitor{memory_ref_cnt}.process(copied_instance);
    copied_instance = class_instance<T>{};
  }

  dl::restore_default_script_allocator(force_enable_allocator);
  return copied_instance;
}

template<class T>
class_instance<T> copy_instance_into_script_memory(const class_instance<T> &instance) noexcept {
  class_instance<T> copied_instance = instance;
  InstanceDeepCopyVisitor copyVisitor{dl::get_default_script_allocator()};
  copyVisitor.process(copied_instance);
  if (unlikely(copyVisitor.is_depth_limit_exceeded())) {
    copied_instance = class_instance<T>{};
  }
  return copied_instance;
}

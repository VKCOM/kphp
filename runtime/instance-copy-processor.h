// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>
#include <unordered_map>
#include <utility>

#include "common/mixin/not_copyable.h"

#include "kphp-core/kphp_core.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"
#include "runtime/memory_resource/unsynchronized_pool_resource.h"

namespace impl_ {

template<typename Child>
class InstanceDeepBasicVisitor : vk::not_copyable {
public:
  template<typename T>
  void operator()(const char *, T &&value) noexcept {
    const bool is_ok = child_.process(std::forward<T>(value));
    is_ok_ = is_ok_ && is_ok;
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
  InstanceDeepBasicVisitor(Child &child, ExtraRefCnt memory_ref_cnt = ExtraRefCnt::extra_ref_cnt_value(0)) noexcept:
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
  const ExtraRefCnt memory_ref_cnt_{ExtraRefCnt::extra_ref_cnt_value(0)};
  Child &child_;
};

constexpr static uint32_t VISITED_INSTANCE_MASK{0x80000000};

} // namespace impl_

class InstanceReferencesCountingVisitor : impl_::InstanceDeepBasicVisitor<InstanceReferencesCountingVisitor> {
public:
  friend class impl_::InstanceDeepBasicVisitor<InstanceReferencesCountingVisitor>;

  using Basic = impl_::InstanceDeepBasicVisitor<InstanceReferencesCountingVisitor>;
  using Basic::operator();

  explicit InstanceReferencesCountingVisitor(std::unordered_map<void *, uint32_t> &instances_refcnt_table) :
    Basic(*this),
    instances_refcnt_table(instances_refcnt_table) {
  }

  template<typename I>
  bool process_instance(class_instance<I> &instance) noexcept {
    return process(instance);
  }

private:
  std::unordered_map<void *, uint32_t> &instances_refcnt_table;

  template<typename T>
  bool process(T &t) noexcept {
    return is_class_instance_inside<T>{} ? Basic::process(t) : true;
  }

  template<typename T>
  std::enable_if_t<is_class_instance_inside<T>{}, bool> process(array<T> &arr) noexcept {
    Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
    return true;
  }

  template<typename I>
  bool process(class_instance<I> &instance) noexcept {
    if (!instance.is_null()) {
      uint32_t &refcnt_info = instances_refcnt_table[instance.get()->get_instance_data_raw_ptr()];
      const bool visited = ++refcnt_info & impl_::VISITED_INSTANCE_MASK;
      refcnt_info |= impl_::VISITED_INSTANCE_MASK;
      if (visited) {
        return true;
      }
    }
    return Basic::process(instance);
  }
};

using ResourceCallbackOOM = bool (*)(memory_resource::unsynchronized_pool_resource &, size_t);

class InstanceDeepCopyVisitor : impl_::InstanceDeepBasicVisitor<InstanceDeepCopyVisitor> {
public:
  friend class impl_::InstanceDeepBasicVisitor<InstanceDeepCopyVisitor>;

  using Basic = impl_::InstanceDeepBasicVisitor<InstanceDeepCopyVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::get_memory_ref_cnt;

  InstanceDeepCopyVisitor(memory_resource::unsynchronized_pool_resource &memory_pool,
                          ExtraRefCnt memory_ref_cnt = ExtraRefCnt::extra_ref_cnt_value(0),
                          ResourceCallbackOOM oom_callback = nullptr) noexcept;

  template<class T>
  bool process(array<T> &arr) noexcept {
    return process_array(arr);
  }

  bool process(string &str) noexcept;

  bool is_memory_limit_exceeded() const noexcept {
    return memory_limit_exceeded_;
  }

  bool is_ok() const noexcept {
    return Basic::is_ok() && !is_memory_limit_exceeded();
  }

  bool is_enough_memory_for(size_t size) noexcept {
    if (!memory_limit_exceeded_) {
      if (!memory_pool_.is_enough_memory_for(size)) {
        if (!oom_callback_) {
          memory_limit_exceeded_ = true;
        } else {
          memory_limit_exceeded_ = !oom_callback_(memory_pool_, size);
        }
      }
    }
    return !memory_limit_exceeded_;
  }

  void *prepare_raw_memory(size_t size) noexcept {
    return is_enough_memory_for(size) ? memory_pool_.allocate(size) : nullptr;
  }

  template<class I>
  bool process_instance(class_instance<I> &instance) noexcept {
    class_instance<I> instance_copy = instance;
    const bool result = process(instance);
    copied_instances_table.clear();
    return result;
  }

private:
  template<class I>
  bool process(class_instance<I> &instance) noexcept {
    if (instance.is_null()) {
      return true;
    }

    auto &copied_instance_ptr = copied_instances_table[instance.get()->get_instance_data_raw_ptr()];

    if (copied_instance_ptr) {
      instance = class_instance<I>::create_from_base_raw_ptr(copied_instance_ptr);
      return true;
    }

    if (unlikely(!is_enough_memory_for(instance.estimate_memory_usage()))) {
      instance = class_instance<I>{};
      return false;
    }

    instance = instance.virtual_builtin_clone();
    copied_instance_ptr = instance.get_base_raw_ptr();

    if (const auto extra_ref_cnt = get_memory_ref_cnt()) {
      instance.set_reference_counter_to(extra_ref_cnt);
    }
    return Basic::process(instance);
  }

  template<class T>
  struct PrimitiveArrayProcessor {
    static bool process(InstanceDeepCopyVisitor &self, array<T> &arr) noexcept;
  };

  template<class T>
  using is_primitive = vk::is_type_in_list<T, int64_t, double, bool, Optional<int64_t>, Optional<double>, Optional<bool>>;

  template<class T>
  bool process_array_impl(array<T> &arr) noexcept {
    if (arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      return true;
    }
    if (unlikely(!is_enough_memory_for(arr.calculate_memory_for_copying()))) {
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
    if (const auto extra_ref_cnt = get_memory_ref_cnt()) {
      arr.set_reference_counter_to(extra_ref_cnt);
    }
    const bool primitive_array = is_primitive<T>{} && arr.has_no_string_keys();
    return primitive_array || Basic::process_range(first, arr.end());
  }

  template<class T>
  std::enable_if_t<!is_primitive<T>{}, bool> process_array(array<T> &arr) noexcept {
    return process_array_impl(arr);
  }

  template<class T>
  std::enable_if_t<is_primitive<T>{}, bool> process_array(array<T> &arr) noexcept {
    return PrimitiveArrayProcessor<T>::process(*this, arr);
  }

  bool memory_limit_exceeded_{false};
  memory_resource::unsynchronized_pool_resource &memory_pool_;
  ResourceCallbackOOM oom_callback_{nullptr};

  dl::CriticalSectionGuard guard; // as we use STL container
  std::unordered_map<void *, void *> copied_instances_table;
};

class InstanceDeepDestroyVisitor : impl_::InstanceDeepBasicVisitor<InstanceDeepDestroyVisitor> {
public:
  friend class impl_::InstanceDeepBasicVisitor<InstanceDeepDestroyVisitor>;

  using Basic = impl_::InstanceDeepBasicVisitor<InstanceDeepDestroyVisitor>;
  using Basic::process;
  using Basic::operator();
  using Basic::is_ok;

  explicit InstanceDeepDestroyVisitor(ExtraRefCnt memory_ref_cnt) noexcept;

  template<typename T>
  bool process(array<T> &arr) noexcept {
    // if array is constant, skip it, otherwise element was cached and should be destroyed
    if (!arr.is_reference_counter(ExtraRefCnt::for_global_const)) {
      Basic::process_range(arr.begin_no_mutate(), arr.end_no_mutate());
      arr.force_destroy(get_memory_ref_cnt());
    }
    arr = array<T>{};
    return true;
  }

  bool process(string &str) noexcept;

  template<class I>
  bool process_instance(class_instance<I> &instance) noexcept {
    InstanceReferencesCountingVisitor{instances_refcnt_table}.process_instance(instance);
    auto res = process(instance);
    instances_refcnt_table.clear();
    return res;
  }

private:
  dl::CriticalSectionGuard guard; // as we use STL container
  std::unordered_map<void *, uint32_t> instances_refcnt_table;

  template<typename I>
  bool process(class_instance<I> &instance) noexcept {
    if (instance.is_null()) {
      return true;
    }

    uint32_t &refcnt_info = instances_refcnt_table[instance.get()->get_instance_data_raw_ptr()];
    if (refcnt_info & impl_::VISITED_INSTANCE_MASK) {
      refcnt_info ^= impl_::VISITED_INSTANCE_MASK;
      Basic::process(instance);
    }

    if (!--refcnt_info) {
      instance.force_destroy(get_memory_ref_cnt());
    }
    instance = class_instance<I>{};
    return true;
  }
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
    detach_processor.process_instance(detached_instance);

    const auto memory_ref_cnt = detach_processor.get_memory_ref_cnt();

    // sizeof(size_t) - an extra memory that we need inside the make_unique_on_script_memory
    constexpr auto size_for_wrapper = sizeof(size_t) + sizeof(InstanceCopyistImpl<class_instance<I>>);
    if (unlikely(!detach_processor.is_ok() || !detach_processor.is_enough_memory_for(size_for_wrapper))) {
      InstanceDeepDestroyVisitor{memory_ref_cnt}.process_instance(detached_instance);
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
      InstanceDeepDestroyVisitor{static_cast<ExtraRefCnt::extra_ref_cnt_value>(memory_ref_cnt_)}.process_instance(instance_);
    }
  }

private:
  class_instance<I> instance_;
  const int memory_ref_cnt_{0};
};

template<class T>
class_instance<T> copy_instance_into_other_memory(const class_instance<T> &instance,
                                                  memory_resource::unsynchronized_pool_resource &memory_pool,
                                                  ExtraRefCnt memory_ref_cnt = ExtraRefCnt{ExtraRefCnt::extra_ref_cnt_value(0)},
                                                  ResourceCallbackOOM oom_callback = nullptr) noexcept {
  dl::MemoryReplacementGuard shared_memory_guard{memory_pool};

  class_instance<T> copied_instance = instance;
  InstanceDeepCopyVisitor copyVisitor{memory_pool, memory_ref_cnt, oom_callback};
  copyVisitor.process_instance(copied_instance);
  if (unlikely(!copyVisitor.is_ok())) {
    copied_instance = class_instance<T>{};
  }

  return copied_instance;
}

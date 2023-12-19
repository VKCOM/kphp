// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <stack>
#include <tuple>
#include <type_traits>
#include <unordered_set>

#include "common/mixin/not_copyable.h"
#include "common/type_traits/list_of_types.h"
#include "runtime/declarations.h"
#include "runtime/kphp_core.h"
#include "runtime/shape.h"

template<class T>
struct CDataPtr;

template<class T>
struct CDataRef;

class CommonMemoryEstimateVisitor : vk::not_copyable {
public:
  template<typename T>
  void operator()(const char *name [[maybe_unused]], T &&value) noexcept {
    process(std::forward<T>(value));
    auto &[size, clazz] = last_instance_element.top();
    if (clazz < static_cast<const void *>(&value)) {
      clazz = &value;
      size = static_cast<int64_t>(sizeof(value));
    }
  }

  template<typename T>
  void process(const CDataRef<T> &value [[maybe_unused]]) {
    estimated_elements_memory_ += sizeof(void *);
  }

  template<typename T>
  void process(const CDataPtr<T> &value [[maybe_unused]]) {}

  void process(const string &value) {
    if (value.is_reference_counter(ExtraRefCnt::for_global_const) || value.is_reference_counter(ExtraRefCnt::for_instance_cache)) {
      return;
    }
    estimated_elements_memory_ += value.estimate_memory_usage();
  }

  void process(const mixed &value) {
    if (value.is_string()) {
      process(value.as_string());
    }
    if (value.is_array()) {
      process(value.as_array());
    }
  }

  template<typename T, typename = vk::enable_if_in_list<T, vk::list_of_types<bool, int64_t, double, Unknown, void *>>>
  void process(const T &value [[maybe_unused]]) {}

  template<typename T>
  void process(const array<T> &value) {
    if (value.is_reference_counter(ExtraRefCnt::for_global_const) || value.is_reference_counter(ExtraRefCnt::for_instance_cache)) {
      return;
    }
    estimated_elements_memory_ += static_cast<int64_t>(value.estimate_memory_usage());
    for (auto it = value.cbegin(); it != value.cend(); ++it) {
      process(it.get_key());
      process(it.get_value());
    }
  }

  template<typename T>
  void process(const Optional<T> &value) {
    if (value.has_value()) {
      process(value.val());
    }
  }

  template<typename... Args>
  void process(const std::tuple<Args...> &value) {
    for_each(value, [this](const auto &v) { this->process(v); });
  }

  template<size_t... Is, typename... T>
  void process(const shape<std::index_sequence<Is...>, T...> &value) {
    (process(value.template get<Is>()), ...);
  }

  template<typename T>
  void process(const class_instance<T> &value) {
    last_instance_element.push({});
    estimated_elements_memory_ += estimate_class_instance_memory_usage(value, std::is_empty<T>{});
    last_instance_element.pop();
  }

  int64_t get_estimated_memory() const {
    return estimated_elements_memory_;
  }

private:
  template<size_t Index = 0, typename Fn, typename... Args>
  std::enable_if_t<Index == sizeof...(Args), void> for_each(const std::tuple<Args...> &value [[maybe_unused]], const Fn &func [[maybe_unused]]) {}

  template<size_t Index = 0, typename Fn, typename... Args>
  std::enable_if_t<Index != sizeof...(Args), void> for_each(const std::tuple<Args...> &value, const Fn &func) {
    func(std::get<Index>(value));
    for_each<Index + 1, Fn, Args...>(value, func);
  }

  template<typename T>
  int64_t estimate_class_instance_memory_usage(const class_instance<T> &value [[maybe_unused]], std::true_type) const {
    return 0;
  }

  template<typename T>
  int64_t estimate_class_instance_memory_usage(const class_instance<T> &value, std::false_type) {
    if (value.is_null() || value.is_reference_counter(ExtraRefCnt::for_instance_cache)) {
      return 0;
    }

    if (!processed_instances.insert(value.get()->get_instance_data_raw_ptr()).second) {
      return 0;
    }
    value.get()->accept(*this);
    return this->get_instance_estimated_memory_impl(value.get(), std::is_polymorphic<T>{});
  }

  template<typename ClassType>
  int64_t get_instance_estimated_memory_impl(const ClassType *obj [[maybe_unused]], std::false_type) const {
    return sizeof(ClassType);
  }

  template<typename ClassType>
  int64_t get_instance_estimated_memory_impl(const ClassType *obj, std::true_type) const {
    // TODO: virtual sizeof?
    // empty polymorphic class
    const auto [size, clazz] = last_instance_element.top();
    if (clazz == nullptr) {
      // vtable + counter
      return 16;
    }
    const auto *class_begin = static_cast<const uint8_t *>(dynamic_cast<const void *>(obj));
    int64_t class_size = static_cast<const uint8_t *>(clazz) - class_begin;
    php_assert(class_size > 0);
    class_size += size;
    // all polymorphic classes are aligned by 8 (vtable ptr)
    class_size = (class_size + 7) & -8;
    return class_size;
  }

  struct instance_element {
    int64_t size = 0;
    const void *clazz = nullptr;
  };

  int64_t estimated_elements_memory_{0};

  dl::CriticalSectionGuard guard;
  std::unordered_set<void *> processed_instances;
  std::stack<instance_element> last_instance_element;
};

template<typename T>
int64_t f$estimate_memory_usage(const T &value) {
  CommonMemoryEstimateVisitor visitor;
  visitor.process(value);
  return visitor.get_estimated_memory();
}

template<typename Int = int64_t, typename = std::enable_if_t<std::is_same<int64_t, Int>{}>>
array<int64_t> f$get_global_vars_memory_stats(Int limit = 0);

template<typename Int, typename>
array<int64_t> f$get_global_vars_memory_stats(Int limit) {
  array<int64_t> get_global_vars_memory_stats_impl(int64_t) noexcept;
  return get_global_vars_memory_stats_impl(limit);
}

// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <numeric>
#include "runtime/kphp_core.h"
#include "runtime/shape.h"

int64_t f$estimate_memory_usage(const string &value);

int64_t f$estimate_memory_usage(const mixed &value);

template<typename T,
  typename = vk::enable_if_in_list<T, vk::list_of_types<bool, int64_t, double, Unknown>>>
int64_t f$estimate_memory_usage(const T &);

template<typename T>
int64_t f$estimate_memory_usage(const array<T> &value);

template<typename T>
int64_t f$estimate_memory_usage(const Optional<T> &value);

template<typename ...Args>
int64_t f$estimate_memory_usage(const std::tuple<Args...> &value);

template<size_t ...Is, typename ...T>
int64_t f$estimate_memory_usage(const shape<std::index_sequence<Is...>, T...> &value);

template<typename T>
int64_t f$estimate_memory_usage(const class_instance<T> &value);

template<typename Int = int64_t, typename = std::enable_if_t<std::is_same<int64_t, Int>{}>>
array<int64_t> f$get_global_vars_memory_stats(Int limit = 0);

template<typename T, typename>
int64_t f$estimate_memory_usage(const T &) {
  return 0;
}

template<typename T>
int64_t f$estimate_memory_usage(const array<T> &value) {
  if (value.is_reference_counter(ExtraRefCnt::for_global_const) || value.is_reference_counter(ExtraRefCnt::for_instance_cache)) {
    return 0;
  }
  int64_t result = static_cast<int64_t>(value.estimate_memory_usage());
  for (auto it = value.cbegin(); it != value.cend(); ++it) {
    result += f$estimate_memory_usage(it.get_key()) + f$estimate_memory_usage(it.get_value());
  }
  return result;
}

template<typename T>
int64_t f$estimate_memory_usage(const Optional<T> &value) {
  return value.has_value() ? f$estimate_memory_usage(value.val()) : 0;
}

namespace impl_ {

template<size_t Index = 0, typename ...Args>
std::enable_if_t<Index == sizeof...(Args), int64_t> estimate_tuple_memory_usage(const std::tuple<Args...> &) {
  return 0;
}

template<size_t Index = 0, typename ...Args>
std::enable_if_t<Index != sizeof...(Args), int64_t> estimate_tuple_memory_usage(const std::tuple<Args...> &value) {
  return estimate_tuple_memory_usage<Index + 1>(value) + f$estimate_memory_usage(std::get<Index>(value));
}

} // namespace impl_

template<typename ...Args>
int64_t f$estimate_memory_usage(const std::tuple<Args...> &value) {
  return impl_::estimate_tuple_memory_usage(value);
}

template<size_t ...Is, typename ...T>
int64_t f$estimate_memory_usage(const shape<std::index_sequence<Is...>, T...> &value) {
  int64_t memory[] = {f$estimate_memory_usage(value.template get<Is>())...};
  return std::accumulate(std::begin(memory), std::end(memory), 0);
}

class InstanceMemoryEstimateVisitor {
public:
  template<typename T>
  void operator()(const char *, const T &value) {
    estimated_elements_memory_ += f$estimate_memory_usage(value);
    if (class_last_element_ < static_cast<const void *>(&value)) {
      class_last_element_ = &value;
      last_element_size_ = static_cast<int64_t>(sizeof(value));
    }
  }

  template<typename ClassType>
  int64_t get_estimated_memory(const ClassType *obj) const {
    return get_estimated_memory_impl(obj, std::is_polymorphic<ClassType>{});
  }

private:
  template<typename ClassType>
  int64_t get_estimated_memory_impl(const ClassType *, std::false_type) const {
    return static_cast<int64_t>(sizeof(ClassType)) + estimated_elements_memory_;
  }

  template<typename ClassType>
  int64_t get_estimated_memory_impl(const ClassType *obj, std::true_type) const {
    // TODO: virtual sizeof?
    // empty polymorphic class
    if (!class_last_element_) {
      // vtable + counter
      return estimated_elements_memory_ + 16;
    }
    const uint8_t *class_begin = static_cast<const uint8_t *>(dynamic_cast<const void *>(obj));
    int64_t class_size = static_cast<const uint8_t *>(class_last_element_) - class_begin;
    php_assert(class_size > 0);
    class_size += last_element_size_;
    // all polymorphic classes are aligned by 8 (vtable ptr)
    class_size = (class_size + 7) & -8;
    return class_size + estimated_elements_memory_;
  }

  int64_t estimated_elements_memory_{0};
  const void *class_last_element_{nullptr};
  int64_t last_element_size_{0};
};

template<typename T>
int64_t estimate_class_instance_memory_usage(const class_instance<T> &, std::true_type) {
  return 0;
}

template<typename T>
int64_t estimate_class_instance_memory_usage(const class_instance<T> &value, std::false_type) {
  if (value.is_null() || value.is_cache_reference_counter()) {
    return 0;
  }

  InstanceMemoryEstimateVisitor visitor;
  value.get()->accept(visitor);
  return visitor.get_estimated_memory(value.get());
}

template<typename T>
int64_t f$estimate_memory_usage(const class_instance<T> &value) {
  return estimate_class_instance_memory_usage(value, std::is_empty<T>{});
}

template<typename Int, typename>
array<int64_t> f$get_global_vars_memory_stats(Int lower_bound) {
  array<int64_t> get_global_vars_memory_stats_impl(int64_t);
  return get_global_vars_memory_stats_impl(lower_bound);
}

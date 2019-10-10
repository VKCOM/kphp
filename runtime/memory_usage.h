#pragma once

#include "runtime/kphp_core.h"

int f$estimate_memory_usage(const string &value);

int f$estimate_memory_usage(const var &value);

template<typename T, typename = enable_for_bool_int_double<T>>
int f$estimate_memory_usage(const T &);

template<typename T>
int f$estimate_memory_usage(const array<T> &value);

template<typename T>
int f$estimate_memory_usage(const OrFalse<T> &value);

template<typename ...Args>
int f$estimate_memory_usage(const std::tuple<Args...> &value);

template<typename T>
int f$estimate_memory_usage(const class_instance<T> &value);


template<typename T, typename>
int f$estimate_memory_usage(const T &) {
  return 0;
}

template<typename T>
int f$estimate_memory_usage(const array<T> &value) {
  int result = static_cast<int>(value.estimate_memory_usage());
  for (auto it = value.cbegin(); it != value.cend(); ++it) {
    result += f$estimate_memory_usage(it.get_key()) + f$estimate_memory_usage(it.get_value());
  }
  return result;
}

template<typename T>
int f$estimate_memory_usage(const OrFalse<T> &value) {
  return value.bool_value ? f$estimate_memory_usage(value.val()) : 0;
}

namespace {

template<size_t Index = 0, typename ...Args>
std::enable_if_t<Index == sizeof...(Args), int> estimate_tuple_memory_usage(const std::tuple<Args...> &) {
  return 0;
}

template<size_t Index = 0, typename ...Args>
std::enable_if_t<Index != sizeof...(Args), int> estimate_tuple_memory_usage(const std::tuple<Args...> &value) {
  return estimate_tuple_memory_usage<Index + 1>(value) + f$estimate_memory_usage(std::get<Index>(value));
}

}

template<typename ...Args>
int f$estimate_memory_usage(const std::tuple<Args...> &value) {
  return estimate_tuple_memory_usage(value);
}

class InstanceMemoryEstimateVisitor {
public:
  template<typename T>
  void operator()(const char *, const T &value) {
    estimated_memory_ += f$estimate_memory_usage(value);
  }

  int get_estimated_memory() const {
    return estimated_memory_;
  }

private:
  int estimated_memory_{0};
};

template<typename T>
int f$estimate_memory_usage(const class_instance<T> &value) {
  if (value.is_null()) {
    return 0;
  }

  InstanceMemoryEstimateVisitor visitor;
  value.get()->accept(visitor);
  return visitor.get_estimated_memory() + value.estimate_memory_usage();
}

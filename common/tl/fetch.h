// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "common/tl/parse.h"
#include "common/type_traits/list_of_types.h"

namespace vk {
namespace tl {

constexpr int max_vector_len = 100000;
constexpr int max_string_len = (1U << 24U);

template<class Allocator>
int fetch_string(std::basic_string<char, std::char_traits<char>, Allocator>& res, int max_len = max_string_len) {
  int len = tl_fetch_string_len(max_len);
  if (len < 0) {
    return -1;
  }
  res.resize(static_cast<size_t>(len));
  if (tl_fetch_string_data(&res[0], len) < 0) {
    return -1;
  }
  return len;
}

namespace detail {

template<typename T, bool isFloatingPoint = std::is_floating_point<T>::value>
struct value_to_string {
  static std::string get(T value) {
    return std::to_string(value);
  }
};

template<typename T>
struct value_to_string<T, true> {
  static std::string get(T value) {
    const size_t buff_size = 100;
    static char buff[buff_size];
    int len = snprintf(buff, buff_size, "%.15lf", static_cast<double>(value));
    if (len < 0) {
      return "";
    }
    return std::string(buff, len);
  }
};

template<typename T>
using good_for_raw_fetch = vk::is_type_in_list<T, int32_t, uint32_t, int64_t, uint64_t, double, long long, unsigned long long>;
} // namespace detail

template<typename T, bool = detail::good_for_raw_fetch<T>::value>
struct fetcher;

template<typename T>
struct fetcher<T, true> {
  void operator()(T& value) const {
    if (tl_fetch_check(sizeof(T)) < 0) {
      value = -1;
      return;
    }
    tl_fetch_raw_data(&value, sizeof(T));
  }
};

template<typename T>
struct fetcher<T, false> {
  void operator()(T& value) const {
    value.tl_fetch();
  }
};

template<typename T, typename = std::enable_if_t<detail::good_for_raw_fetch<T>::value>>
struct in_range_fetcher {
  static constexpr T eps = (std::is_floating_point<T>::value) ? (1e-9) : (0);

  explicit in_range_fetcher(T min_val = std::numeric_limits<T>::min(), T max_val = std::numeric_limits<T>::max())
      : min_val{min_val},
        max_val{max_val} {}

  void operator()(T& value) const {
    if (tl_fetch_check(sizeof(T)) < 0) {
      value = -1;
      return;
    }
    tl_fetch_raw_data(&value, sizeof(T));
    if (!(value + eps >= min_val && value - eps <= max_val)) {
      tl_fetch_set_error_format(TL_ERROR_VALUE_NOT_IN_RANGE, "Expected value (typeid: '%s') in range [%s,%s], %s presented", typeid(T).name(),
                                detail::value_to_string<T>::get(min_val).c_str(), detail::value_to_string<T>::get(max_val).c_str(),
                                detail::value_to_string<T>::get(value).c_str());
    }
  }

  const T min_val;
  const T max_val;
};

template<typename Allocator>
struct fetcher<std::basic_string<char, std::char_traits<char>, Allocator>, false> {
  explicit fetcher(int max_len = max_string_len)
      : max_len{max_len} {}

  void operator()(std::basic_string<char, std::char_traits<char>, Allocator>& s) const {
    fetch_string(s, max_len);
  }

  const int max_len;
};

template<typename T1, typename T2>
struct fetcher<std::pair<T1, T2>, false> {
  void operator()(std::pair<T1, T2>& pair) const {
    fetcher<T1>()(pair.first);
    fetcher<T2>()(pair.second);
  }
};

namespace detail {

template<typename T, typename Allocator, typename Fetcher>
bool fetch_tuple_impl(std::vector<T, Allocator>& vec, size_t len, Fetcher&& fetcher_func, std::false_type) {
  vec.resize(len);
  for (auto& item : vec) {
    fetcher_func(item);
  }
  return !tl_fetch_error();
}

template<typename T, typename Allocator, typename Fetcher>
bool fetch_tuple_impl(std::vector<T, Allocator>& vec, size_t len, Fetcher&&, std::true_type) {
  vec.resize(len);
  assert(sizeof(T) * len <= std::numeric_limits<int>::max());
  tl_fetch_data(vec.data(), static_cast<int>(sizeof(T) * len));
  return !tl_fetch_error();
}

template<typename T, typename Allocator, typename Fetcher>
bool fetch_tuple(std::vector<T, Allocator>& vec, size_t len, Fetcher&& fetcher_func) {
  if (len > std::numeric_limits<int>::max()) {
    tl_fetch_set_error_format(TL_ERROR_BAD_VALUE, "len should be between %d and %d (but it is %lu)", 0, std::numeric_limits<int>::max(), len);
    return false;
  }
  static constexpr int MAX_ARRAY_LEN_WITHOUT_TL_FETCH_CHECK = 1000000;
  // Проверяем что размер вектора не слишком большой.
  // Идея в том, что можно построить оценку сверху на максимальный размер вектора из количества пришедших байт.
  // Такая оценка будет верна всегда, кроме когда вектор хранит пустой тип (%True или при fields_mask = 0).
  // Для этого есть оценка снизу на проверяемый размер.
  if (len > MAX_ARRAY_LEN_WITHOUT_TL_FETCH_CHECK && tl_fetch_check(static_cast<int>(len) * 4) < 0) {
    return false;
  }
  return fetch_tuple_impl(vec, len, std::forward<Fetcher>(fetcher_func), std::integral_constant < bool,
                          std::is_same<std::decay_t<Fetcher>, fetcher<T>>{} && detail::good_for_raw_fetch<T>{} > {});
}

} // namespace detail

template<typename T, typename Allocator, typename Fetcher = fetcher<T>>
bool fetch_vector(std::vector<T, Allocator>& vec, int max_len = max_vector_len, Fetcher&& fetcher_func = Fetcher{}) {
  int len = tl_fetch_int_range(0, max_len);
  if (tl_fetch_error()) {
    return false;
  }
  return detail::fetch_tuple(vec, len, std::forward<Fetcher>(fetcher_func));
}

template<typename T, typename Allocator, typename Fetcher = fetcher<T>>
bool fetch_tuple(std::vector<T, Allocator>& vec, size_t len, Fetcher&& fetcher_func = Fetcher{}) {
  return detail::fetch_tuple(vec, len, std::forward<Fetcher>(fetcher_func));
}

static inline bool fetch_magic(int expected_magic) {
  int real_magic = tl_fetch_int();
  if (real_magic != expected_magic) {
    tl_fetch_set_error_format(TL_ERROR_SYNTAX, "unexpected magic %08x instead of %08x", real_magic, expected_magic);
    return false;
  }
  return true;
}

} // namespace tl
} // namespace vk

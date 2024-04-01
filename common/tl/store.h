// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "common/tl/constants/common.h"
#include "common/tl/parse.h"
#include "common/type_traits/list_of_types.h"
#include "common/type_traits/range_value_type.h"
#include "common/wrappers/span.h"
#include "common/wrappers/string_view.h"

namespace vk {
namespace tl {

void inline store_string(string_view s) {
  return tl_store_string(s.data(), static_cast<int>(s.size()));
}

namespace detail {
template<typename T>
using good_for_raw_store = vk::is_type_in_list<T, int32_t, uint32_t, int64_t, uint64_t, double, long long, unsigned long long>;
} // namespace detail

template<typename T, bool = detail::good_for_raw_store<T>::value>
struct storer;

template<typename T>
struct storer<T, true> {
  void operator()(T value) const {
    tl_store_raw_data(&value, sizeof(T));
  }
};

template<typename T>
struct storer<T, false> {
  void operator()(const T &value) const {
    value.tl_store();
  }
};

template<typename Allocator>
struct storer<std::basic_string<char, std::char_traits<char>, Allocator>, false> {
  void operator()(const std::basic_string<char, std::char_traits<char>, Allocator> &s) const {
    store_string(s);
  }
};

template<typename T1, typename T2>
struct storer<std::pair<T1, T2>, false> {
  void operator()(const std::pair<T1, T2> &s) const {
    storer<T1>()(s.first);
    storer<T2>()(s.second);
  }
};

template<typename T, typename Storer = storer<T>>
void store_optional(const std::optional<T> &value, Storer &&store_func = {}) {
  if (value.has_value()) {
    tl_store_int(TL_MAYBE_TRUE);
    store_func(*value);
  } else {
    tl_store_int(TL_MAYBE_FALSE);
  }
}

template<typename T>
struct storer<std::optional<T>, false> {
  void operator()(const std::optional<T> &s) const {
    store_optional(s);
  }
};

namespace detail {

template<typename Range, typename Storer = storer<range_value_type<Range>>>
size_t store_vector_impl(bool store_size, Range &&range, Storer &&store_func, std::false_type, std::false_type) {
  auto first = std::begin(std::forward<Range>(range));
  auto last = std::end(std::forward<Range>(range));

  int local_size{};
  int &size = store_size ? *reinterpret_cast<int *>(tl_store_get_ptr(sizeof(int))) : local_size;
  size = 0;
  std::for_each(first, last, [&store_func, &size](const range_value_type<Range> &value) {
    if (store_func(value)) {
      assert(size < std::numeric_limits<int>::max());
      size++;
    }
  });
  return size;
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
size_t store_vector_impl(bool store_size, Range &&range, Storer &&store_func, std::false_type, std::true_type) {
  auto first = std::begin(std::forward<Range>(range));
  auto last = std::end(std::forward<Range>(range));

  size_t size = std::distance(first, last);
  assert(size <= std::numeric_limits<int>::max());
  if (store_size) {
    tl_store_int(static_cast<int>(size));
  }
  std::for_each(first, last, std::forward<Storer>(store_func));
  return size;
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
size_t store_vector_impl(bool store_size, Range &&range, Storer &&, std::true_type, std::true_type) {
  vk::span<const range_value_type<Range>> span(std::forward<Range>(range));
  size_t size = span.size();
  assert(size <= std::numeric_limits<int>::max());
  if (store_size) {
    tl_store_int(static_cast<int>(size));
  }
  assert(span.size() * sizeof(range_value_type<Range>) <= std::numeric_limits<int>::max());
  tl_store_raw_data(span.data(), static_cast<int>(span.size() * sizeof(range_value_type<Range>)));
  return size;
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
size_t store_vector(bool store_size, Range &&range, Storer &&store_func = {}) {
  return detail::store_vector_impl(store_size, std::forward<Range>(range), std::forward<Storer>(store_func), std::integral_constant < bool,
                                   std::is_same<std::decay_t<Storer>, storer<range_value_type<Range>>>{}
                                     && detail::good_for_raw_store<range_value_type<Range>>{}
                                     && std::is_constructible<vk::span<const range_value_type<Range>>, Range &&>{} > {},
                                   std::is_same<std::invoke_result_t<Storer, const range_value_type<Range> &>, void>{});
}

} // namespace detail

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_tuple(size_t expected_size, Range &&range, Storer &&store_func = {}) {
  size_t size = detail::store_vector(false, std::forward<Range>(range), std::forward<Storer>(store_func));
  if (size != expected_size) {
    tl_fetch_set_error_format(TL_ERROR_BAD_VALUE, "Tuple size error: expected %lu, actual %lu", expected_size, size);
  }
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_tuple_with_magic(size_t expected_size, Range &&range, Storer &&store_func = {}) {
  tl_store_int(TL_TUPLE);
  return store_tuple(expected_size, std::forward<Range>(range), std::forward<Storer>(store_func));
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_vector(Range &&range, Storer &&store_func = {}) {
  detail::store_vector(true, std::forward<Range>(range), std::forward<Storer>(store_func));
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_vector_with_magic(Range &&range, Storer &&store_func = {}) {
  tl_store_int(TL_VECTOR);
  return store_vector(std::forward<Range>(range), std::forward<Storer>(store_func));
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_vector_total(int total, Range &&range, Storer &&store_func = {}) {
  tl_store_int(total);
  store_vector(range, store_func);
}

template<typename Range, typename Storer = storer<range_value_type<Range>>>
void store_vector_total_with_magic(int total, Range &&range, Storer &&store_func = {}) {
  tl_store_int(TL_VECTOR_TOTAL);
  store_vector_total(total, std::forward<Range>(range), std::forward<Storer>(store_func));
}

template<typename T, typename Allocator>
struct storer<std::vector<T, Allocator>, false> {
  void operator()(const std::vector<T, Allocator> &s) const {
    store_vector(s);
  }
};

} // namespace tl
} // namespace vk

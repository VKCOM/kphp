// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/array/array-functions.h"

#include <cinttypes>
#include <cstdint>
#include <memory>

#include "runtime-common/core/runtime-core.h"

namespace {

template<class FN>
void walk_parts(const char *d, int64_t d_len, const string &str, int64_t limit, FN handle_part) noexcept {
  const char *s = str.c_str();
  int64_t s_len = str.size();
  int64_t prev = 0;

  if (d_len == 1) {
    // a faster path for 1-char delimiter (a very frequent case)
    if (limit > 1) {
      char delimiter_char = d[0];
      for (string::size_type i = 0; i < s_len;) {
        if (delimiter_char == s[i]) {
          handle_part(s + prev, static_cast<string::size_type>(i - prev));
          i++;
          prev = i;
          limit--;
          if (limit == 1) {
            break;
          }
        } else {
          i++;
        }
      }
    }
    handle_part(s + prev, static_cast<string::size_type>(s_len - prev));
    return;
  }

  if (limit > 1) {
    for (int64_t i = 0; i < s_len;) {
      int64_t j = 0;
      for (j = 0; j < d_len && d[j] == s[i + j]; j++) {
      }
      if (j == d_len) {
        handle_part(s + prev, static_cast<string::size_type>(i - prev));
        i += d_len;
        prev = i;
        limit--;
        if (limit == 1) {
          break;
        }
      } else {
        i++;
      }
    }
  }
  handle_part(s + prev, static_cast<string::size_type>(s_len - prev));
}

} // namespace

namespace array_functions_impl_ {

string implode_string_vector(const string &s, const array<string> &a) noexcept {
  // we use a precondition here that array is not empty: count-1 is not negative, elems[0] is valid
  int64_t count = a.count();
  const string *elems = a.get_const_vector_pointer();

  int result_size = 0;
  for (int64_t i = 0; i < count; i++) {
    result_size += elems[i].size();
  }

  if (s.empty()) {
    // ~5-10% of implode usages involve an empty delimiter: implode("", $arr)
    // people use it to have a fast way to concatenate a large number of strings together
    string result(result_size, true);
    for (int64_t i = 0; i < count; i++) {
      result.append_unsafe(elems[i]);
    }
    return result.finish_append();
  }

  result_size += s.size() * (count - 1);
  string result(result_size, true);
  result.append_unsafe(elems[0]);
  for (int64_t i = 1; i < count; i++) {
    result.append_unsafe(s).append_unsafe(elems[i]);
  }
  return result.finish_append();
}

} // namespace array_functions_impl_

string f$_explode_nth(const string &delimiter, const string &str, int64_t index) noexcept {
  if (delimiter.empty()) {
    php_warning("Empty delimiter in function explode");
    return {};
  }
  int result_index = 0;
  string res;
  walk_parts(delimiter.c_str(), delimiter.size(), str, index + 2, [&](const char *s, string::size_type l) noexcept {
    if (result_index++ == index) {
      res = string(s, l);
    }
  });
  return res;
}

string f$_explode_1(const string &delimiter, const string &str) noexcept {
  return f$_explode_nth(delimiter, str, 0);
}

std::tuple<string, string> f$_explode_tuple2(const string &delimiter, const string &str, int64_t mask, int64_t limit) noexcept {
  if (delimiter.empty()) {
    php_warning("Empty delimiter in function explode");
    return {};
  }
  int result_index = 0;
  std::tuple<string, string> res;
  walk_parts(delimiter.c_str(), delimiter.size(), str, limit, [&](const char *s, string::size_type l) {
    int index = result_index++;
    if (((1 << index) & mask) == 0) {
      return; // this result index will be discarded, don't allocate the string
    }
    switch (index) {
      case 0:
        std::get<0>(res) = string(s, l);
        return;
      case 1:
        std::get<1>(res) = string(s, l);
        return;
      default:
        return;
    }
  });
  return res;
}

std::tuple<string, string, string> f$_explode_tuple3(const string &delimiter, const string &str, int64_t mask, int64_t limit) noexcept {
  if (delimiter.empty()) {
    php_warning("Empty delimiter in function explode");
    return {};
  }
  int result_index = 0;
  std::tuple<string, string, string> res;
  walk_parts(delimiter.c_str(), delimiter.size(), str, limit, [&](const char *s, string::size_type l) {
    int index = result_index++;
    if (((1 << index) & mask) == 0) {
      return; // this result index will be discarded, don't allocate the string
    }
    switch (index) {
      case 0:
        std::get<0>(res) = string(s, l);
        return;
      case 1:
        std::get<1>(res) = string(s, l);
        return;
      case 2:
        std::get<2>(res) = string(s, l);
        return;
      default:
        return;
    }
  });
  return res;
}

std::tuple<string, string, string, string> f$_explode_tuple4(const string &delimiter, const string &str, int64_t mask, int64_t limit) noexcept {
  if (delimiter.empty()) {
    php_warning("Empty delimiter in function explode");
    return {};
  }
  int result_index = 0;
  std::tuple<string, string, string, string> res;
  walk_parts(delimiter.c_str(), delimiter.size(), str, limit, [&](const char *s, string::size_type l) {
    int index = result_index++;
    if (((1 << index) & mask) == 0) {
      return; // this result index will be discarded, don't allocate the string
    }
    switch (index) {
      case 0:
        std::get<0>(res) = string(s, l);
        return;
      case 1:
        std::get<1>(res) = string(s, l);
        return;
      case 2:
        std::get<2>(res) = string(s, l);
        return;
      case 3:
        std::get<3>(res) = string(s, l);
        return;
      default:
        return;
    }
  });
  return res;
}
array<string> explode(char delimiter, const string &str, int64_t limit) noexcept {
  array<string> res(array_size(limit < 10 ? limit : 1, true));
  walk_parts(std::addressof(delimiter), 1, str, limit, [&](const char *s, string::size_type l) { res.push_back(string(s, l)); });
  return res;
}

array<string> f$explode(const string &delimiter, const string &str, int64_t limit) noexcept {
  if (limit < 1) {
    php_warning("Wrong limit %" PRIi64 " specified in function explode", limit);
    limit = 1;
  }
  if (delimiter.empty()) {
    php_warning("Empty delimiter in function explode");
    return {};
  }

  array<string> res(array_size(limit < 10 ? limit : 1, true));
  walk_parts(delimiter.c_str(), delimiter.size(), str, limit, [&](const char *s, string::size_type l) { res.push_back(string(s, l)); });
  return res;
}
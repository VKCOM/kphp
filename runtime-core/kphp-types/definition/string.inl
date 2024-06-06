// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cctype>

#include "common/algorithms/simd-int-to-string.h"

#include "runtime-core/utils/migration-php8.h"
#include "runtime-core/kphp-types/definition/string_cache.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

namespace __runtime_core {
template<typename Allocator>
tmp_string<Allocator>::tmp_string(const char *data, string_size_type size)
  : data{data}
  , size{size} {}

template<typename Allocator>
tmp_string<Allocator>::tmp_string(const string<Allocator> &s)
  : data{s.c_str()}
  , size{s.size()} {}

template<typename Allocator>
bool string<Allocator>::string_inner::is_shared() const {
  return ref_count > 0;
}

template<typename Allocator>
void string<Allocator>::string_inner::set_length_and_sharable(size_type n) {
  //  fprintf (stderr, "inc ref cnt %d %s\n", 0, ref_data());
  ref_count = 0;
  size = n;
  ref_data()[n] = '\0';
}

template<typename Allocator>
char *string<Allocator>::string_inner::ref_data() const {
  return (char *)(this + 1);
}

template<typename Allocator>
typename string<Allocator>::size_type string<Allocator>::string_inner::new_capacity(size_type requested_capacity, size_type old_capacity) {
  if (requested_capacity > max_size()) {
    php_critical_error("tried to allocate too big string of size %lld", (long long)requested_capacity);
  }

  if (requested_capacity > old_capacity && requested_capacity < 2 * old_capacity) {
    requested_capacity = 2 * old_capacity;
  }

  const size_type page_size = 4096;
  size_type new_size = (size_type)((requested_capacity + 1) + sizeof(string_inner));

  if (new_size > page_size && requested_capacity > old_capacity) {
    requested_capacity += (new_size + page_size - 1) / page_size * page_size - new_size;
  }

  return requested_capacity;
}

template<typename Allocator>
typename string<Allocator>::string_inner *string<Allocator>::string_inner::create(size_type requested_capacity, size_type old_capacity) {
  size_type capacity = new_capacity(requested_capacity, old_capacity);
  size_type new_size = (size_type)(sizeof(string_inner) + (capacity + 1));
  string_inner *p = (string_inner *)Allocator::allocate(new_size);
  p->capacity = capacity;
  return p;
}

template<typename Allocator>
char *string<Allocator>::string_inner::reserve(size_type requested_capacity) {
  size_type new_cap = new_capacity(requested_capacity, capacity);
  size_type old_size = (size_type)(sizeof(string_inner) + (capacity + 1));
  size_type new_size = (size_type)(sizeof(string_inner) + (new_cap + 1));

  string_inner *p = (string_inner *)Allocator::reallocate((void *)this, new_size, old_size);
  p->capacity = new_cap;
  return p->ref_data();
}

template<typename Allocator>
void string<Allocator>::string_inner::dispose() {
  //  fprintf (stderr, "dec ref cnt %d %s\n", ref_count - 1, ref_data());
  if (ref_count < ExtraRefCnt::for_global_const) {
    ref_count--;
    if (ref_count <= -1) {
      destroy();
    }
  }
}

template<typename Allocator>
void string<Allocator>::string_inner::destroy() {
  Allocator::deallocate(this, get_memory_usage());
}

template<typename Allocator>
inline typename string<Allocator>::size_type string<Allocator>::string_inner::get_memory_usage() const {
  return static_cast<size_type>(sizeof(string_inner) + (capacity + 1));
}

template<typename Allocator>
char *string<Allocator>::string_inner::ref_copy() {
  //  fprintf (stderr, "inc ref cnt %d, %s\n", ref_count + 1, ref_data());
  if (ref_count < ExtraRefCnt::for_global_const) {
    ref_count++;
  }
  return ref_data();
}

template<typename Allocator>
char *string<Allocator>::string_inner::clone(size_type requested_cap) {
  string_inner *r = string_inner::create(requested_cap, capacity);
  if (size) {
    memcpy(r->ref_data(), ref_data(), size);
  }

  r->set_length_and_sharable(size);
  return r->ref_data();
}

template<typename Allocator>
typename string<Allocator>::string_inner *string<Allocator>::inner() const {
  return (string<Allocator>::string_inner *)p - 1;
}

template<typename Allocator>
bool string<Allocator>::disjunct(const char *s) const {
  return (s < p || p + size() < s);
}

template<typename Allocator>
void string<Allocator>::set_size(size_type new_size) {
  if (inner()->is_shared()) {
    string_inner *r = string_inner::create(new_size, capacity());

    inner()->dispose();
    p = r->ref_data();
  } else if (new_size > capacity()) {
    p = inner()->reserve(new_size);
  }
  inner()->set_length_and_sharable(new_size);
}

template<typename Allocator>
char *string<Allocator>::create(const char *beg, const char *end) {
  const size_type dnew = static_cast<size_type>(end - beg);
  if (dnew == 0) {
    return string_cache<Allocator>::empty_string().ref_data();
  }
  if (dnew == 1) {
    return string_cache<Allocator>::cached_char(*beg).ref_data();
  }

  string_inner *r = string_inner::create(dnew, 0);
  char *s = r->ref_data();
  memcpy(s, beg, dnew);

  r->ref_count = 0;
  r->size = dnew;
  s[dnew] = '\0';
  return s;
}

template<typename Allocator>
char *string<Allocator>::create(size_type n, char c) {
  if (n == 0) {
    return string_cache<Allocator>::empty_string().ref_data();
  }
  if (n == 1) {
    return string_cache<Allocator>::cached_char(c).ref_data();
  }

  string_inner *r = string_inner::create(n, 0);
  char *s = r->ref_data();
  memset(s, c, n);
  r->set_length_and_sharable(n);
  return s;
}

template<typename Allocator>
char *string<Allocator>::create(size_type n, bool b) {
  string_inner *r = string_inner::create(n, 0);

  if (b) {
    r->ref_count = 0;
    r->size = 0;
  } else {
    r->set_length_and_sharable(n);
  }
  return r->ref_data();
}

template<typename Allocator>
string<Allocator>::string()
  : p(string_cache<Allocator>::empty_string().ref_data()) {}

template<typename Allocator>
string<Allocator>::string(const string &str) noexcept
  : p(str.inner()->ref_copy()) {}

template<typename Allocator>
string<Allocator>::string(string &&str) noexcept
  : p(str.p) {
  str.p = string_cache<Allocator>::empty_string().ref_data();
}

template<typename Allocator>
string<Allocator>::string(const char *s, size_type n)
  : p(create(s, s + n)) {}

template<typename Allocator>
string<Allocator>::string(const char *s)
  : string(s, static_cast<size_type>(strlen(s))) {}

template<typename Allocator>
string<Allocator>::string(size_type n, char c)
  : p(create(n, c)) {}

template<typename Allocator>
string<Allocator>::string(size_type n, bool b)
  : p(create(n, b)) {}

template<typename Allocator>
string<Allocator>::string(int64_t i) {
  if (i >= 0 && i < string_cache<Allocator>::cached_int_max()) {
    p = string_cache<Allocator>::cached_int(i).ref_data();
    return;
  }
  if (i < 0 && i > -string_cache<Allocator>::cached_int_max()) {
    const auto &cached_int_positive = string_cache<Allocator>::cached_int(-i);
    p = create(cached_int_positive.size + 1, true);
    p[0] = '-';
    // copy with \0
    std::memcpy(p + 1, cached_int_positive.ref_data(), cached_int_positive.size + 1);
    inner()->size = cached_int_positive.size + 1;
    return;
  }

  const auto i32 = static_cast<int32_t>(i);
  const char *end = nullptr;
  if (static_cast<int64_t>(i32) == i) {
    p = create(STRLEN_INT32, true);
    end = simd_int32_to_string(i32, p);
  } else {
    p = create(STRLEN_INT64, true);
    end = simd_int64_to_string(i, p);
  }
  inner()->size = static_cast<size_type>(end - p);
  p[inner()->size] = '\0';
}

template<typename Allocator>
string<Allocator>::string(double f) {
  constexpr uint32_t MAX_LEN = 4096;
  char result[MAX_LEN + 2];
  result[0] = '\0';
  result[1] = '\0';

  char *begin = result + 2;
  if (std::isnan(f)) {
    // to prevent printing `-NAN` by snprintf
    f = std::abs(f);
  }
  int len = snprintf(begin, MAX_LEN, "%.14G", f);
  if (static_cast<uint32_t>(len) < MAX_LEN) {
    if (static_cast<uint32_t>(begin[len - 1] - '5') < 5 && begin[len - 2] == '0' && begin[len - 3] == '-') {
      --len;
      begin[len - 1] = begin[len];
    }
    if (begin[1] == 'E') {
      result[0] = begin[0];
      result[1] = '.';
      result[2] = '0';
      begin = result;
      len += 2;
    } else if (begin[0] == '-' && begin[2] == 'E') {
      result[0] = begin[0];
      result[1] = begin[1];
      result[2] = '.';
      result[3] = '0';
      begin = result;
      len += 2;
    }
    php_assert(len <= STRLEN_FLOAT);
    p = create(begin, begin + len);
  } else {
    php_warning("Maximum length of float (%d) exceeded", MAX_LEN);
    p = string_cache<Allocator>::empty_string().ref_data();
  }
}

template<typename Allocator>
string<Allocator>::string(ArrayBucketDummyStrTag) noexcept
  : p(nullptr) {}

template<typename Allocator>
bool string<Allocator>::is_dummy_string() const noexcept {
  return p == nullptr;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::operator=(const string &str) noexcept {
  return assign(str);
}

template<typename Allocator>
string<Allocator> &string<Allocator>::operator=(string &&str) noexcept {
  if (this != &str) {
    destroy();
    p = str.p;
    str.p = string_cache<Allocator>::empty_string().ref_data();
  }
  return *this;
}

template<typename Allocator>
typename string<Allocator>::size_type string<Allocator>::size() const {
  return inner()->size;
}

template<typename Allocator>
void string<Allocator>::shrink(size_type n) {
  const size_type old_size = size();

  if (n < old_size) {
    if (inner()->is_shared()) {
      string_inner *r = string_inner::create(n, capacity());

      if (n) {
        memcpy(r->ref_data(), p, n);
      }

      inner()->dispose();
      p = r->ref_data();
    }
    inner()->set_length_and_sharable(n);
  }
}

template<typename Allocator>
typename string<Allocator>::size_type string<Allocator>::capacity() const {
  return inner()->capacity;
}

template<typename Allocator>
void string<Allocator>::make_not_shared() {
  if (inner()->is_shared()) {
    force_reserve(size());
  }
}

template<typename Allocator>
string<Allocator> string<Allocator>::copy_and_make_not_shared() const {
  string result = *this;
  result.make_not_shared();
  return result;
}

template<typename Allocator>
void string<Allocator>::force_reserve(size_type res) {
  char *new_p = inner()->clone(res);
  inner()->dispose();
  p = new_p;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::reserve_at_least(size_type res) {
  if (inner()->is_shared()) {
    force_reserve(res);
  } else if (res > capacity()) {
    p = inner()->reserve(res);
  }
  return *this;
}

template<typename Allocator>
bool string<Allocator>::empty() const {
  return size() == 0;
}

template<typename Allocator>
bool string<Allocator>::starts_with(const string<Allocator> &other) const noexcept {
  return other.size() > size() ? false : !memcmp(c_str(), other.c_str(), other.size());
}

template<typename Allocator>
bool string<Allocator>::ends_with(const string<Allocator> &other) const noexcept {
  return other.size() > size() ? false : !memcmp(c_str() + (size() - other.size()), other.c_str(), other.size());
}

template<typename Allocator>
const char &string<Allocator>::operator[](size_type pos) const {
  return p[pos];
}

template<typename Allocator>
char &string<Allocator>::operator[](size_type pos) {
  return p[pos];
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(const string<Allocator> &str) {
  const size_type n2 = str.size();
  if (n2) {
    const size_type len = n2 + size();
    reserve_at_least(len);

    memcpy(p + size(), str.p, n2);
    inner()->set_length_and_sharable(len);
  }
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(const string<Allocator> &str, size_type pos2, size_type n2) {
  if (pos2 > str.size()) {
    pos2 = str.size();
  }
  if (n2 > str.size() - pos2) {
    n2 = str.size() - pos2;
  }
  if (n2) {
    const size_type len = n2 + size();
    reserve_at_least(len);

    memcpy(p + size(), str.p + pos2, n2);
    inner()->set_length_and_sharable(len);
  }
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(const char *s) {
  return append(s, static_cast<size_type>(strlen(s)));
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(const char *s, size_type n) {
  if (n) {
    if (max_size() - size() < n) {
      php_critical_error("tried to allocate too big string of size %lld", (long long)size() + n);
    }
    const size_type len = n + size();
    if (len > capacity() || inner()->is_shared()) {
      if (disjunct(s)) {
        force_reserve(len);
      } else {
        const size_type off = (size_type)(s - p);
        force_reserve(len);
        s = p + off;
      }
    }
    memcpy(p + size(), s, n);
    inner()->set_length_and_sharable(len);
  }

  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(size_type n, char c) {
  if (n) {
    if (max_size() - size() < n) {
      php_critical_error("tried to allocate too big string of size %lld", (long long)size() + n);
    }
    const size_type len = n + size();
    reserve_at_least(len);
    memset(p + size(), c, n);
    inner()->set_length_and_sharable(len);
  }

  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(bool b) {
  if (b) {
    push_back('1');
  }

  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(int64_t i) {
  if (i >= 0 && i < string_cache<Allocator>::cached_int_max()) {
    const auto &cached_int = string_cache<Allocator>::cached_int(i);
    return append(cached_int.ref_data(), cached_int.size);
  }
  if (i < 0 && i > -string_cache<Allocator>::cached_int_max()) {
    const auto &cached_int_positive = string_cache<Allocator>::cached_int(-i);
    reserve_at_least(size() + cached_int_positive.size + 1);
    p[inner()->size++] = '-';
    append_unsafe(cached_int_positive.ref_data(), cached_int_positive.size);
    return finish_append();
  }

  reserve_at_least(size() + STRLEN_INT64);
  const char *end = simd_int64_to_string(i, p + size());
  inner()->size = static_cast<size_type>(end - p);
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(double d) {
  return append(string<Allocator>(d));
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append(const mixed<Allocator> &v) {
  switch (v.get_type()) {
    case mixed<Allocator>::type::NUL:
      return *this;
    case mixed<Allocator>::type::BOOLEAN:
      if (v.as_bool()) {
        push_back('1');
      }
      return *this;
    case mixed<Allocator>::type::INTEGER:
      return append(v.as_int());
    case mixed<Allocator>::type::FLOAT:
      return append(string(v.as_double()));
    case mixed<Allocator>::type::STRING:
      return append(v.as_string());
    case mixed<Allocator>::type::ARRAY:
      php_warning("Conversion from array to string");
      return append("Array", 5);
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
void string<Allocator>::push_back(char c) {
  const size_type len = 1 + size();
  reserve_at_least(len);
  p[len - 1] = c;
  inner()->set_length_and_sharable(len);
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(bool b) {
  if (b) {
    p[inner()->size++] = '1';
  }
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(int64_t i) {
  if (i >= 0 && i < string_cache<Allocator>::cached_int_max()) {
    const auto &cached_int = string_cache<Allocator>::cached_int(i);
    return append_unsafe(cached_int.ref_data(), cached_int.size);
  }
  if (i < 0 && i > -string_cache<Allocator>::cached_int_max()) {
    const auto &cached_int_positive = string_cache<Allocator>::cached_int(-i);
    p[inner()->size++] = '-';
    return append_unsafe(cached_int_positive.ref_data(), cached_int_positive.size);
  }

  const char *end = simd_int64_to_string(i, p + size());
  inner()->size = static_cast<size_type>(end - p);
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(double d) {
  return append_unsafe(string<Allocator>(d)); // TODO can be optimized
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(const string<Allocator> &str) {
  const size_type n2 = str.size();
  memcpy(p + size(), str.p, n2);
  inner()->size += n2;

  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(tmp_string<Allocator> str) {
  if (!str.has_value()) {
    return *this;
  }
  return append_unsafe(str.data, str.size);
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(const char *s, size_type n) {
  memcpy(p + size(), s, n);
  inner()->size += n;

  return *this;
}

template<typename Allocator>
template<class T>
string<Allocator> &string<Allocator>::append_unsafe(const array<T, Allocator> &a __attribute__((unused))) {
  php_warning("Conversion from array to string");
  return append_unsafe("Array", 5);
}

template<typename Allocator>
string<Allocator> &string<Allocator>::append_unsafe(const mixed<Allocator> &v) {
  switch (v.get_type()) {
    case mixed<Allocator>::type::NUL:
      return *this;
    case mixed<Allocator>::type::BOOLEAN:
      return append_unsafe(v.as_bool());
    case mixed<Allocator>::type::INTEGER:
      return append_unsafe(v.as_int());
    case mixed<Allocator>::type::FLOAT:
      return append_unsafe(v.as_double());
    case mixed<Allocator>::type::STRING:
      return append_unsafe(v.as_string());
    case mixed<Allocator>::type::ARRAY:
      return append_unsafe(v.as_array());
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
string<Allocator> &string<Allocator>::finish_append() {
  php_assert(inner()->size <= inner()->capacity);
  p[inner()->size] = '\0';
  return *this;
}

template<typename Allocator>
template<class T>
string<Allocator> &string<Allocator>::append_unsafe(const Optional<T> &v) {
  if (v.has_value()) {
    return append_unsafe(v.val());
  }
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(const string<Allocator> &str) {
  char *str_copy = str.inner()->ref_copy();
  destroy();
  p = str_copy;
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(const string<Allocator> &str, size_type pos, size_type n) {
  if (pos > str.size()) {
    pos = str.size();
  }
  if (n > str.size() - pos) {
    n = str.size() - pos;
  }
  return assign(str.p + pos, n);
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(const char *s) {
  return assign(s, static_cast<size_type>(strlen(s)));
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(const char *s, size_type n) {
  if (max_size() < n) {
    php_critical_error("tried to allocate too big string of size %lld", (long long)n);
  }

  if (disjunct(s) || inner()->is_shared()) {
    set_size(n);
    memcpy(p, s, n);
  } else {
    const size_type pos = (size_type)(s - p);
    if (pos >= n) {
      memcpy(p, s, n);
    } else if (pos) {
      memmove(p, s, n);
    }
    inner()->set_length_and_sharable(n);
  }
  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(size_type n, char c) {
  if (max_size() < n) {
    php_critical_error("tried to allocate too big string of size %lld", (long long)n);
  }

  set_size(n);
  memset(p, c, n);

  return *this;
}

template<typename Allocator>
string<Allocator> &string<Allocator>::assign(size_type n, bool b __attribute__((unused))) {
  if (max_size() < n) {
    php_critical_error("tried to allocate too big string of size %lld", (long long)n);
  }

  set_size(n);

  return *this;
}

template<typename Allocator>
void string<Allocator>::assign_raw(const char *s) {
  static_assert(sizeof(string_inner) == 12u, "need 12 bytes");
  p = const_cast<char *>(s + sizeof(string_inner));
}

template<typename Allocator>
void string<Allocator>::swap(string<Allocator> &s) {
  char *tmp = p;
  p = s.p;
  s.p = tmp;
}

template<typename Allocator>
char *string<Allocator>::buffer() {
  return p;
}

template<typename Allocator>
const char *string<Allocator>::c_str() const {
  return p;
}

template<typename Allocator>
inline void string<Allocator>::warn_on_float_conversion() const {
  const char *s = c_str();
  while (isspace(*s)) {
    s++;
  }

  if (s[0] == '-' || s[0] == '+') {
    s++;
  }

  int i = 0;
  while (i < 15) {
    if (s[i] == 0) {
      return;
    }
    if ('0' <= s[i] && s[i] <= '9') {
      i++;
      continue;
    }

    if (s[i] == '.') {
      do {
        i++;
      } while (s[i] == '0');

      if (s[i] == 0) {
        return;
      }

      while (i < 16) {
        if (s[i] == 0) {
          return;
        }

        if ('0' <= s[i] && s[i] <= '9') {
          i++;
          continue;
        }
        break;
      }
    }
    break;
  }

  php_warning("Possible loss of precision on converting string \"%s\" to float", c_str());
}

template<typename Allocator>
inline bool string<Allocator>::try_to_int(int64_t *val) const {
  return php_try_to_int(p, size(), val);
}

template<typename Allocator>
bool string<Allocator>::try_to_float_as_php8(double *val) const {
  if (empty() || (size() >= 2 && p[0] == '0' && vk::any_of_equal(p[1], 'x', 'X'))) {
    return false;
  }
  char *end_ptr = nullptr;
  *val = strtod(p, &end_ptr);

  // If end_ptr == p then this means that strtod could not perform
  // the conversion (otherwise end_ptr would have shifted relative
  // to p), which means it is not a valid number.
  return end_ptr != p && std::all_of(end_ptr, p + size(), [](char c) { return isspace(static_cast<unsigned char>(c)); });
}

template<typename Allocator>
bool string<Allocator>::try_to_float_as_php7(double *val) const {
  if (empty() || (size() >= 2 && p[0] == '0' && vk::any_of_equal(p[1], 'x', 'X'))) {
    return false;
  }
  char *end_ptr{nullptr};
  *val = strtod(p, &end_ptr);
  return (end_ptr == p + size());
}

template<typename Allocator>
bool string<Allocator>::try_to_float(double *val, bool php8_warning) const {
  const bool is_float_php7 = try_to_float_as_php7(val);

  if ((KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_TO_FLOAT_FLAG) && php8_warning) {
    const bool is_float_php8 = try_to_float_as_php8(val);

    if (is_float_php7 != is_float_php8) {
      php_warning("Result of checking that the string is numeric in PHP 7 and PHP 8 are different for '%s' (PHP7: %s, PHP8: %s)\n"
                  "Why does the warning appear:\n"
                  "- string-string comparison\n"
                  "- string-number comparison",
                  p, is_float_php7 ? "true" : "false", is_float_php8 ? "true" : "false");
    }
  }

  return is_float_php7;
}

template<typename Allocator>
mixed<Allocator> string<Allocator>::to_numeric() const {
  double res = to_float();
  constexpr double int_max = static_cast<double>(std::numeric_limits<int64_t>::max());
  constexpr double int_min = static_cast<double>(std::numeric_limits<int64_t>::min());
  if (int_min <= res && res <= int_max) {
    const int int_res = static_cast<int>(res);
    if (int_res == res) {
      return int_res;
    }
  }
  return res;
}

template<typename Allocator>
bool string<Allocator>::to_bool() const {
  return string_to_bool(p, size());
}

template<typename Allocator>
int64_t string<Allocator>::to_int(const char *s, size_type l) {
  while (isspace(*s) && l > 0) {
    s++;
    l--;
  }

  int64_t mul = 1, cur = 0;
  if (l > 0 && (s[0] == '-' || s[0] == '+')) {
    if (s[0] == '-') {
      mul = -1;
    }
    cur++;
  }

  int64_t val = 0;
  while (cur < l && '0' <= s[cur] && s[cur] <= '9') {
    val = val * 10 + s[cur++] - '0';
  }

  return val * mul;
}

template<typename Allocator>
int64_t string<Allocator>::to_int() const {
  return to_int(p, size());
}

template<typename Allocator>
int64_t string<Allocator>::to_int(int64_t start, int64_t l) const {
  return to_int(p + start, l);
}

template<typename Allocator>
double string<Allocator>::to_float() const {
  double res{0};
  try_to_float(&res, false); // it's ok if float number was parsed partially
  return res;
}

template<typename Allocator>
const string<Allocator> &string<Allocator>::to_string() const {
  return *this;
}

template<typename Allocator>
int64_t string<Allocator>::safe_to_int() const {
  int64_t mul = 1;
  size_type l = size();
  size_type cur = 0;
  if (l > 0 && (p[0] == '-' || p[0] == '+')) {
    if (p[0] == '-') {
      mul = -1;
    }
    cur++;
  }

  uint64_t val = 0;
  bool overfow = false;
  while (cur < l && '0' <= p[cur] && p[cur] <= '9') {
    val = val * 10 + p[cur] - '0';
    if (val > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
      overfow = true;
    }
  }

  if (overfow) {
    php_warning("Integer overflow on converting string \"%s\" to int", c_str());
  }
  return static_cast<int64_t>(val) * mul;
}

template<typename Allocator>
bool string<Allocator>::is_int() const {
  return php_is_int(p, size());
}

template<typename Allocator>
bool string<Allocator>::is_numeric_as_php8() const {
  const char *s = c_str();
  while (isspace(*s)) {
    s++;
  }

  if (*s == '+' || *s == '-') {
    s++;
  }

  int l = 0;
  while (*s >= '0' && *s <= '9') {
    l++;
    s++;
  }

  if (*s == '.') {
    s++;
    while (*s >= '0' && *s <= '9') {
      l++;
      s++;
    }
  }

  if (l == 0) {
    return false;
  }

  if (*s == 'e' || *s == 'E') {
    s++;
    if (*s == '+' || *s == '-') {
      s++;
    }

    if (*s == '\0') {
      return false;
    }

    while (*s >= '0' && *s <= '9') {
      s++;
    }
  }

  return std::all_of(s, c_str() + size(), [](const char c) { return isspace(static_cast<unsigned char>(c)); });
}

template<typename Allocator>
bool string<Allocator>::is_numeric_as_php7() const {
  // using default PHP7-based function;
  // this function is also used by the compiler
  return php_is_numeric(c_str());
}

template<typename Allocator>
bool string<Allocator>::is_numeric() const {
  const auto php7_result = is_numeric_as_php7();

  if (KphpCoreContext::current().show_migration_php8_warning & MIGRATION_PHP8_STRING_TO_FLOAT_FLAG) {
    const bool php8_result = is_numeric_as_php8();

    if (php7_result != php8_result) {
      php_warning("is_numeric('%s') result in PHP 7 and PHP 8 are different (PHP7: %s, PHP8: %s)", p, php7_result ? "true" : "false",
                  php8_result ? "true" : "false");
    }
  }

  return php7_result;
}

template<typename Allocator>
int64_t string<Allocator>::hash() const {
  return string_hash(p, size());
}

template<typename Allocator>
string<Allocator> string<Allocator>::substr(size_type pos, size_type n) const {
  return string<Allocator>(p + pos, n);
}

template<typename Allocator>
typename string<Allocator>::size_type string<Allocator>::find(const string<Allocator> &s, size_type pos) const {
  for (size_type i = pos; i + s.size() <= size(); i++) {
    bool equal = true;
    for (size_type j = 0; j < s.size(); j++) {
      if (p[i + j] != s.p[j]) {
        equal = false;
        break;
      }
    }
    if (equal) {
      return i;
    }
  }
  return string<Allocator>::npos;
}

template<typename Allocator>
typename string<Allocator>::size_type string<Allocator>::find_first_of(const string<Allocator> &s, size_type pos) const {
  for (size_type i = pos; i < size(); i++) {
    for (size_type j = 0; j < s.size(); j++) {
      if (p[i] == s.p[j]) {
        return i;
      }
    }
  }
  return string<Allocator>::npos;
}

template<typename Allocator>
int64_t string<Allocator>::compare(const string<Allocator> &str, const char *other, size_type other_size) {
  const size_type my_size = str.size();
  const int res = memcmp(str.p, other, std::min(my_size, other_size));
  return res ? res : static_cast<int64_t>(my_size) - static_cast<int64_t>(other_size);
}

template<typename Allocator>
int64_t string<Allocator>::compare(const string<Allocator> &str) const {
  return compare(*this, str.c_str(), str.size());
}

template<typename Allocator>
bool string<Allocator>::isset(int64_t index) const {
  return index >= 0 && index < size();
}

template<typename Allocator>
int64_t string<Allocator>::get_correct_index(int64_t index) const {
  return index >= 0 ? index : index + int64_t{size()};
}

template<typename Allocator>
int64_t string<Allocator>::get_correct_offset(int64_t offset) const {
  return ::get_correct_offset(size(), offset);
}

template<typename Allocator>
int64_t string<Allocator>::get_correct_offset_clamped(int64_t offset) const {
  if (offset < 0) {
    offset = offset + size();
    if (offset < 0) {
      offset = 0;
    }
  } else if (offset > size()) {
    offset = size();
  }
  return offset;
}

template<typename Allocator>
const string<Allocator> string<Allocator>::get_value(int64_t int_key) const {
  const int64_t true_key = get_correct_index(int_key);
  if (unlikely(!isset(true_key))) {
    return {};
  }
  return string(1, p[true_key]);
}

template<typename Allocator>
const string<Allocator> string<Allocator>::get_value(const string<Allocator> &string_key) const {
  int64_t int_val = 0;
  if (!string_key.try_to_int(&int_val)) {
    php_warning("\"%s\" is illegal offset for string", string_key.c_str());
    int_val = string_key.to_int();
  }
  return get_value(int_val);
}

template<typename Allocator>
const string<Allocator> string<Allocator>::get_value(const mixed<Allocator> &v) const {
  switch (v.get_type()) {
    case mixed<Allocator>::type::NUL:
      return get_value(0);
    case mixed<Allocator>::type::BOOLEAN:
      return get_value(static_cast<int64_t>(v.as_bool()));
    case mixed<Allocator>::type::INTEGER:
      return get_value(v.as_int());
    case mixed<Allocator>::type::FLOAT:
      return get_value(static_cast<int64_t>(v.as_double()));
    case mixed<Allocator>::type::STRING:
      return get_value(v.as_string());
    case mixed<Allocator>::type::ARRAY:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return string<Allocator>();
    default:
      __builtin_unreachable();
  }
}

template<typename Allocator>
int64_t string<Allocator>::get_reference_counter() const {
  return inner()->ref_count + 1;
}

template<typename Allocator>
bool string<Allocator>::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
  return inner()->ref_count == ref_cnt_value;
}

template<typename Allocator>
void string<Allocator>::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
  // some const arrays are placed in read only memory and can't be modified
  if (inner()->ref_count != ref_cnt_value) {
    inner()->ref_count = ref_cnt_value;
  }
}

template<typename Allocator>
void string<Allocator>::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  php_assert(expected_ref_cnt != ExtraRefCnt::for_global_const);
  if (p) {
    php_assert(inner()->ref_count == expected_ref_cnt);
    inner()->ref_count = 0;
    inner()->dispose();
    p = nullptr;
  }
}

template<typename Allocator>
inline typename string<Allocator>::size_type string<Allocator>::estimate_memory_usage() const {
  return inner()->get_memory_usage();
}

template<typename Allocator>
inline typename string<Allocator>::size_type string<Allocator>::estimate_memory_usage(size_t len) noexcept {
  return static_cast<size_type>(sizeof(string_inner)) + string_inner::new_capacity(len, 0);
}

template<typename Allocator>
inline string<Allocator> string<Allocator>::make_const_string_on_memory(const char *str, size_type len, void *memory, size_t memory_size) {
  php_assert(len + inner_sizeof() + 1 <= memory_size);
  auto *inner = new (memory) string_inner{len, len, ExtraRefCnt::for_global_const};
  memcpy(inner->ref_data(), str, len);
  inner->ref_data()[len] = '\0';
  string result;
  result.p = inner->ref_data();
  return result;
}

template<typename Allocator>
inline void string<Allocator>::destroy() {
  if (p) {
    inner()->dispose();
  }
}

template<typename Allocator>
string<Allocator>::~string() noexcept {
  destroy();
}
}

int64_t get_correct_offset(string_size_type size, int64_t offset) {
  if (offset < 0) {
    offset = offset + size;
    if (offset < 0) {
      offset = 0;
    }
  }
  return offset;
}

template<typename Allocator>
bool operator==(const __string<Allocator> &lhs, const __string<Allocator> &rhs) {
  return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template<typename Allocator>
bool operator!=(const __string<Allocator> &lhs, const __string<Allocator> &rhs) {
  return lhs.size() != rhs.size() || lhs.compare(rhs) != 0;
}

bool is_ok_float(double v) {
  return v == 0.0 || (std::fpclassify(v) == FP_NORMAL && v < 1e100 && v > -1e100);
}

template<typename Allocator>
inline bool is_all_digits(const __string<Allocator> &s) {
  for (typename __runtime_core::string<Allocator>::size_type i = 0; i < s.size(); ++i) {
    if (!('0' <= s[i] && s[i] <= '9')) {
      return false;
    }
  }

  return true;
}

template<typename Allocator>
int64_t compare_strings_php_order(const __string<Allocator> &lhs, const __string<Allocator> &rhs) {
  if (lhs.size() == rhs.size() && is_all_digits(lhs) && is_all_digits(rhs)) {
    return lhs.compare(rhs);
  }

  // possible numbers: -.1e2; 0.12; 123; .12; +123; +.123
  bool lhs_maybe_num = lhs[0] <= '9';
  bool rhs_maybe_num = rhs[0] <= '9';

  if (lhs_maybe_num && rhs_maybe_num) {
    int64_t lhs_int_val = 0, rhs_int_val = 0;
    if (lhs.try_to_int(&lhs_int_val) && rhs.try_to_int(&rhs_int_val)) {
      return three_way_comparison(lhs_int_val, rhs_int_val);
    }

    double lhs_float_val = 0, rhs_float_val = 0;
    if (lhs.try_to_float(&lhs_float_val) && rhs.try_to_float(&rhs_float_val)) {
      lhs.warn_on_float_conversion();
      rhs.warn_on_float_conversion();
      if (is_ok_float(lhs_float_val) && is_ok_float(rhs_float_val)) {
        return three_way_comparison(lhs_float_val, rhs_float_val);
      }
    }
  }

  return lhs.compare(rhs);
}

template<typename Allocator>
void swap(__string<Allocator> &lhs, __string<Allocator> &rhs) {
  lhs.swap(rhs);
}


string_size_type max_string_size(bool) {
  return static_cast<string_size_type>(STRLEN_BOOL);
}

string_size_type max_string_size(int32_t) {
  return static_cast<string_size_type>(STRLEN_INT);
}

string_size_type max_string_size(int64_t) {
  return static_cast<string_size_type>(STRLEN_INT);
}

string_size_type max_string_size(double) {
  return static_cast<string_size_type>(STRLEN_FLOAT);
}

template<typename Allocator>
typename __string<Allocator>::size_type max_string_size(const __string<Allocator> &s) {
  return s.size();
}

template<typename Allocator>
typename __string<Allocator>::size_type max_string_size(__runtime_core::tmp_string<Allocator> s) {
  return s.size;
}

template<typename Allocator>
typename __string<Allocator>::size_type max_string_size(const __mixed<Allocator> &v) {
  switch (v.get_type()) {
    case __runtime_core::mixed<Allocator>::type::NUL:
      return 0;
    case __runtime_core::mixed<Allocator>::type::BOOLEAN:
      return max_string_size(v.as_bool());
    case __runtime_core::mixed<Allocator>::type::INTEGER:
      return max_string_size(v.as_int());
    case __runtime_core::mixed<Allocator>::type::FLOAT:
      return max_string_size(v.as_double());
    case __runtime_core::mixed<Allocator>::type::STRING:
      return max_string_size(v.as_string());
    case __runtime_core::mixed<Allocator>::type::ARRAY:
      return max_string_size(v.as_array());
    default:
      __builtin_unreachable();
  }
}

template<class T, typename Allocator>
typename __string<Allocator>::size_type max_string_size(const __array<T, Allocator> &) {
  return static_cast<typename __runtime_core::string<Allocator>::size_type>(STRLEN_ARRAY);
}

template<class T>
string_size_type max_string_size(const Optional<T> &v) {
  return v.has_value() ? max_string_size(v.val()) : 0;
}

// in PHP, the '<' operator when applied to strings tries to compare strings as numbers,
// therefore stl trees with keys of type string run really slow
struct stl_string_less {
  template<typename Allocator>
  bool operator()(const __string<Allocator> &lhs, const __string<Allocator> &rhs) const noexcept {
    return lhs.compare(rhs) < 0;
  }
};

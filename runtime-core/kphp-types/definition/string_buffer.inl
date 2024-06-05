#pragma once

#include "common/algorithms/simd-int-to-string.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

namespace __runtime_core {
template<typename Allocator>
inline void string_buffer<Allocator>::resize(string_size_type new_buffer_len) noexcept {
  string_buffer_lib_context &sb_context = KphpCoreContext::current().sb_lib_context;
  if (new_buffer_len < sb_context.MIN_BUFFER_LEN) {
    new_buffer_len = sb_context.MIN_BUFFER_LEN;
  }
  if (new_buffer_len >= sb_context.MAX_BUFFER_LEN) {
    if (buffer_len + 1 < sb_context.MAX_BUFFER_LEN) {
      new_buffer_len = sb_context.MAX_BUFFER_LEN - 1;
    } else {
      if (sb_context.error_flag != STRING_BUFFER_ERROR_FLAG_OFF) {
        clean();
        sb_context.error_flag = STRING_BUFFER_ERROR_FLAG_FAILED;
        return;
      } else {
        php_critical_error("maximum buffer size exceeded. buffer_len = %u, new_buffer_len = %u", buffer_len, new_buffer_len);
      }
    }
  }

  string_size_type current_len = size();
  if (void *new_mem = Allocator::reallocate(buffer_begin, new_buffer_len, buffer_len)) {
    buffer_begin = static_cast<char *>(new_mem);
    buffer_len = new_buffer_len;
    buffer_end = buffer_begin + current_len;
  }
}

template<typename Allocator>
inline void string_buffer<Allocator>::reserve_at_least(string_size_type need) noexcept {
  string_size_type new_buffer_len = need + size();
  while (unlikely(buffer_len < new_buffer_len && KphpCoreContext::current().sb_lib_context.error_flag != STRING_BUFFER_ERROR_FLAG_FAILED)) {
    resize(((new_buffer_len * 2 + 1 + 64) | 4095) - 64);
  }
}

template<typename Allocator>
string_buffer<Allocator> &string_buffer<Allocator>::clean() noexcept {
  buffer_end = buffer_begin;
  return *this;
}

template<typename Allocator>
string_size_type string_buffer<Allocator>::size() const noexcept {
  return static_cast<string_size_type>(buffer_end - buffer_begin);
}

template<typename Allocator>
char *string_buffer<Allocator>::buffer() {
  return buffer_begin;
}

template<typename Allocator>
const char *string_buffer<Allocator>::buffer() const {
  return buffer_begin;
}

template<typename Allocator>
const char *string_buffer<Allocator>::c_str() {
  reserve_at_least(1);
  *buffer_end = 0;

  return buffer_begin;
}

template<typename SBAllocator>
template<typename StringAllocator>
string<StringAllocator> string_buffer<SBAllocator>::str() const {
  php_assert(size() <= buffer_len);
  return string<StringAllocator>(buffer_begin, size());
}

template<typename Allocator>
bool string_buffer<Allocator>::set_pos(int64_t pos) {
  php_assert(static_cast<string_size_type>(pos) <= buffer_len);
  buffer_end = buffer_begin + pos;
  return true;
}

template<typename Allocator>
string_buffer<Allocator> &string_buffer<Allocator>::append(const char *str, size_t len) noexcept {
  reserve_at_least(static_cast<string_size_type>(len));

  if (unlikely(KphpCoreContext::current().sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return *this;
  }
  memcpy(buffer_end, str, len);
  buffer_end += len;

  return *this;
}

template<typename Allocator>
void string_buffer<Allocator>::write(const char *str, int len) noexcept {
  append(str, len);
}

template<typename Allocator>
void string_buffer<Allocator>::append_unsafe(const char *str, int len) {
  memcpy(buffer_end, str, len);
  buffer_end += len;
}

template<typename Allocator>
void string_buffer<Allocator>::append_char(char c) {
  *buffer_end++ = c;
}

template<typename Allocator>
void string_buffer<Allocator>::reserve(int len) {
  reserve_at_least(len + 1);
}

template<typename Allocator>
void string_buffer<Allocator>::copy_raw_data(const string_buffer &other) {
  clean();
  append(other.str().c_str(), other.size());
}

template<typename Allocator>
void string_buffer<Allocator>::debug_print() const {
  std::for_each(buffer_begin, buffer_end, [](char c) { fprintf(stderr, "%02x ", (int)c); });
  fprintf(stderr, "\n");
}

template<typename Allocator>
string_buffer<Allocator>::string_buffer(string_size_type buffer_len) noexcept
  : buffer_end(static_cast<char *>(Allocator::allocate(buffer_len)))
  , buffer_begin(buffer_end)
  , buffer_len(buffer_len) {}

template<typename Allocator>
string_buffer<Allocator>::~string_buffer() noexcept {
  Allocator::deallocate(buffer_begin, buffer_len);
}

template<typename SB_Allocator>
inline bool operator==(const __string_buffer<SB_Allocator> &lhs, const __string_buffer<SB_Allocator> &rhs) {
  size_t len_l = lhs.buffer_end - lhs.buffer_begin;
  size_t len_r = rhs.buffer_end - rhs.buffer_begin;
  if (len_l != len_r) {
    return false;
  }
  return std::equal(lhs.buffer_begin, lhs.buffer_end, rhs.buffer_begin);
}

template<typename SB_Allocator>
inline bool operator!=(__string_buffer<SB_Allocator> const &lhs, __string_buffer<SB_Allocator> const &rhs) {
  return !(lhs == rhs);
}

template<typename SB_Allocator>
__runtime_core::string_buffer<SB_Allocator> &operator<<(__runtime_core::string_buffer<SB_Allocator> &sb, char c) {
  sb.reserve_at_least(1);

  *sb.buffer_end++ = c;

  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, const char *s) {
  while (*s != 0) {
    sb.reserve_at_least(1);

    *sb.buffer_end++ = *s++;
  }

  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, double f) {
  return sb << string<SB_Allocator>(f);
}

template<typename SBAllocator, typename StringAllocator>
inline __string_buffer<SBAllocator> &operator<<(__string_buffer<SBAllocator> &sb, const __string<StringAllocator> &s) {
  typename __runtime_core::string<SBAllocator>::size_type l = s.size();
  sb.reserve_at_least(l);

  if (unlikely(KphpCoreContext::current().sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return sb;
  }

  memcpy(sb.buffer_end, s.c_str(), l);
  sb.buffer_end += l;

  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, bool x) {
  if (x) {
    sb.reserve_at_least(1);
    *sb.buffer_end++ = '1';
  }

  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, int32_t x) {
  sb.reserve_at_least(11);
  sb.buffer_end = simd_int32_to_string(x, sb.buffer_end);
  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, uint32_t x) {
  sb.reserve_at_least(10);
  sb.buffer_end = simd_uint32_to_string(x, sb.buffer_end);
  return sb;
}

template<typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, int64_t x) {
  sb.reserve_at_least(20);
  sb.buffer_end = simd_int64_to_string(x, sb.buffer_end);
  return sb;
}
}

template<class T, typename SB_Allocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, const Optional<T> &v) {
  auto write_lambda = [&sb](const auto &v) -> __string_buffer<SB_Allocator>& { return sb << v; };
  return call_fun_on_optional_value(write_lambda, v);
}

template<typename SB_Allocator, typename MixedAllocator>
inline __string_buffer<SB_Allocator> &operator<<(__string_buffer<SB_Allocator> &sb, const __mixed<MixedAllocator> &v) {
  switch (v.get_type()) {
    case __mixed<MixedAllocator>::type::NUL:
      return sb;
    case __mixed<MixedAllocator>::type::BOOLEAN:
      return sb << v.as_bool();
    case __mixed<MixedAllocator>::type::INTEGER:
      return sb << v.as_int();
    case __mixed<MixedAllocator>::type::FLOAT:
      return sb << __string<MixedAllocator>(v.as_double());
    case __mixed<MixedAllocator>::type::STRING:
      return sb << v.as_string();
    case __mixed<MixedAllocator>::type::ARRAY:
      php_warning("Conversion from array to string");
      return sb.append("Array", 5);
    default:
      __builtin_unreachable();
  }
}

inline void init_string_buffer_lib(string_size_type min_length, string_size_type max_length) {
  string_buffer_lib_context &sb_context = KphpCoreContext::current().sb_lib_context;
  assert(min_length > 0 && max_length > 0);
  sb_context.MIN_BUFFER_LEN = min_length;
  sb_context.MAX_BUFFER_LEN = max_length;
}



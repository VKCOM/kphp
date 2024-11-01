#pragma once

#include "common/algorithms/simd-int-to-string.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

inline void string_buffer::resize(string::size_type new_buffer_len) noexcept {
  string_buffer_lib_context &sb_context = RuntimeContext::get().sb_lib_context;
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
        php_critical_error ("maximum buffer size exceeded. buffer_len = %u, new_buffer_len = %u", buffer_len, new_buffer_len);
      }
    }
  }

  string::size_type current_len = size();
  if(void *new_mem = RuntimeAllocator::get().realloc_global_memory(buffer_begin, new_buffer_len, buffer_len)) {
    buffer_begin = static_cast<char *>(new_mem);
    buffer_len = new_buffer_len;
    buffer_end = buffer_begin + current_len;
  }
}

inline void string_buffer::reserve_at_least(string::size_type need) noexcept {
  string::size_type new_buffer_len = need + size();
  while (unlikely (buffer_len < new_buffer_len && RuntimeContext::get().sb_lib_context.error_flag != STRING_BUFFER_ERROR_FLAG_FAILED)) {
    resize(((new_buffer_len * 2 + 1 + 64) | 4095) - 64);
  }
}

string_buffer &string_buffer::clean() noexcept {
  buffer_end = buffer_begin;
  return *this;
}

string_buffer &operator<<(string_buffer &sb, char c) {
  sb.reserve_at_least(1);

  *sb.buffer_end++ = c;

  return sb;
}


string_buffer &operator<<(string_buffer &sb, const char *s) {
  while (*s != 0) {
    sb.reserve_at_least(1);

    *sb.buffer_end++ = *s++;
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, double f) {
  return sb << string(f);
}

string_buffer &operator<<(string_buffer &sb, const string &s) {
  string::size_type l = s.size();
  sb.reserve_at_least(l);

  if (unlikely (RuntimeContext::get().sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return sb;
  }

  memcpy(sb.buffer_end, s.c_str(), l);
  sb.buffer_end += l;

  return sb;
}

string_buffer &operator<<(string_buffer &sb, bool x) {
  if (x) {
    sb.reserve_at_least(1);
    *sb.buffer_end++ = '1';
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, int32_t x) {
  sb.reserve_at_least(11);
  sb.buffer_end = simd_int32_to_string(x, sb.buffer_end);
  return sb;
}

string_buffer &operator<<(string_buffer &sb, uint32_t x) {
  sb.reserve_at_least(10);
  sb.buffer_end = simd_uint32_to_string(x, sb.buffer_end);
  return sb;
}

string_buffer &operator<<(string_buffer &sb, int64_t x) {
  sb.reserve_at_least(20);
  sb.buffer_end = simd_int64_to_string(x, sb.buffer_end);
  return sb;
}

string::size_type string_buffer::size() const noexcept {
  return static_cast<string::size_type>(buffer_end - buffer_begin);
}

char *string_buffer::buffer() {
  return buffer_begin;
}

const char *string_buffer::buffer() const {
  return buffer_begin;
}

const char *string_buffer::c_str() {
  reserve_at_least(1);
  *buffer_end = 0;

  return buffer_begin;
}

string string_buffer::str() const {
  php_assert (size() <= buffer_len);
  return string(buffer_begin, size());
}

bool string_buffer::set_pos(int64_t pos) {
  php_assert (static_cast<string::size_type>(pos) <= buffer_len);
  buffer_end = buffer_begin + pos;
  return true;
}

string_buffer &string_buffer::append(const char *str, size_t len) noexcept {
  reserve_at_least(static_cast<string::size_type>(len));

  if (unlikely (RuntimeContext::get().sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return *this;
  }
  memcpy(buffer_end, str, len);
  buffer_end += len;

  return *this;
}

void string_buffer::write(const char *str, int len) noexcept {
  append(str, len);
}

void string_buffer::append_unsafe(const char *str, int len) {
  memcpy(buffer_end, str, len);
  buffer_end += len;
}

void string_buffer::append_char(char c) {
  *buffer_end++ = c;
}


void string_buffer::reserve(int len) {
  reserve_at_least(len + 1);
}

inline void init_string_buffer_lib(string::size_type min_length, string::size_type max_length) {
  string_buffer_lib_context &sb_context = RuntimeContext::get().sb_lib_context;
  if (min_length > 0) {
    sb_context.MIN_BUFFER_LEN = min_length;
  }
  if (max_length > 0) {
    sb_context.MAX_BUFFER_LEN = max_length;
  }
}

void string_buffer::copy_raw_data(const string_buffer &other) {
  clean();
  append(other.str().c_str(), other.size());
}

bool operator==(const string_buffer &lhs, const string_buffer &rhs) {
  size_t len_l = lhs.buffer_end - lhs.buffer_begin;
  size_t len_r = rhs.buffer_end - rhs.buffer_begin;
  if (len_l != len_r) {
    return false;
  }
  return std::equal(lhs.buffer_begin, lhs.buffer_end, rhs.buffer_begin);
}

bool operator!=(string_buffer const &lhs, string_buffer const &rhs) {
  return !(lhs == rhs);
}

void string_buffer::debug_print() const {
  std::for_each(buffer_begin, buffer_end, [](char c) {
    fprintf(stderr, "%02x ", (int)c);
  });
  fprintf(stderr, "\n");
}

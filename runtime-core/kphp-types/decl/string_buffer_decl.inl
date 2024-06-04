#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

#define STRING_BUFFER_ERROR_FLAG_ON -1
#define STRING_BUFFER_ERROR_FLAG_OFF 0
#define STRING_BUFFER_ERROR_FLAG_FAILED 1

namespace __runtime_core {
template<typename Allocator>
class string_buffer {
  char *buffer_end;
  char *buffer_begin;
  string<Allocator>::size_type buffer_len;

  inline void resize(string<Allocator>::size_type new_buffer_len) noexcept;
  inline void reserve_at_least(string<Allocator>::size_type new_buffer_len) noexcept;
  string_buffer(const string_buffer &other) = delete;
  string_buffer &operator=(const string_buffer &other) = delete;

public:
  explicit string_buffer(string<Allocator>::size_type buffer_len = 4000) noexcept;

  inline string_buffer &clean() noexcept;

  friend inline string_buffer &operator<<(string_buffer &sb, char c);
  friend inline string_buffer &operator<<(string_buffer &sb, const char *s);
  friend inline string_buffer &operator<<(string_buffer &sb, double f);
  template<typename StringAllocator>
  friend inline string_buffer &operator<<(string_buffer &sb, const string<StringAllocator> &s);
  friend inline string_buffer &operator<<(string_buffer &sb, bool x);
  friend inline string_buffer &operator<<(string_buffer &sb, int32_t x);
  friend inline string_buffer &operator<<(string_buffer &sb, uint32_t x);
  friend inline string_buffer &operator<<(string_buffer &sb, int64_t x);

  inline string_buffer &append(const char *str, size_t len) noexcept;
  inline void write(const char *str, int len) noexcept;

  inline void append_unsafe(const char *str, int len);

  inline void append_char(char c) __attribute__((always_inline)); // unsafe

  inline void reserve(int len);

  inline string<Allocator>::size_type size() const noexcept;

  inline char *buffer();
  inline const char *buffer() const;

  inline const char *c_str();
  template<typename StringAllocator>
  inline string<StringAllocator> str() const;

  inline bool set_pos(int64_t pos);

  ~string_buffer() noexcept;

  friend void init_string_buffer_lib(string_size_type min_length, string_size_type max_length);

  inline void debug_print() const;

  inline void copy_raw_data(const string_buffer &other);

  friend inline bool operator==(const string_buffer &lhs, const string_buffer &rhs);
  friend inline bool operator!=(const string_buffer &lhs, const string_buffer &rhs);
};
}

struct string_buffer_lib_context {
  string_size_type MIN_BUFFER_LEN;
  string_size_type MAX_BUFFER_LEN;
  int error_flag = 0;
};

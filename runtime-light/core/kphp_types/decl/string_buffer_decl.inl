#pragma once

#if !defined(INCLUDED_FROM_KPHP_CORE) && !defined(INCLUDED_FROM_KPHP_CONTEXT)
  #error "this file must be included only from kphp_core.h or context.h"
#endif


class string_buffer {
  // todo:k2 set limits in init
  constexpr static string::size_type MIN_BUFFER_LEN = 1024;
  constexpr static string::size_type MAX_BUFFER_LEN = (1 << 24);
  char *buffer_end;
  char *buffer_begin;
  string::size_type buffer_len;

  inline void resize(string::size_type new_buffer_len) noexcept;
  inline void reserve_at_least(string::size_type new_buffer_len) noexcept;
  string_buffer(const string_buffer &other) = delete;
  string_buffer &operator=(const string_buffer &other) = delete;

public:
  explicit string_buffer(string::size_type buffer_len = 256) noexcept;

  inline string_buffer &clean() noexcept;

  friend inline string_buffer &operator<<(string_buffer &sb, char c);
  friend inline string_buffer &operator<<(string_buffer &sb, const char *s);
  friend inline string_buffer &operator<<(string_buffer &sb, double f);
  friend inline string_buffer &operator<<(string_buffer &sb, const string &s);
  friend inline string_buffer &operator<<(string_buffer &sb, bool x);
  friend inline string_buffer &operator<<(string_buffer &sb, int32_t x);
  friend inline string_buffer &operator<<(string_buffer &sb, uint32_t x);
  friend inline string_buffer &operator<<(string_buffer &sb, int64_t x);

  inline string_buffer &append(const char *str, size_t len) noexcept;
  inline void write(const char *str, int len) noexcept;

  inline void append_unsafe(const char *str, int len);

  inline void append_char(char c) __attribute__ ((always_inline));//unsafe

  inline void reserve(int len);

  inline string::size_type size() const noexcept;

  inline char *buffer();
  inline const char *buffer() const;

  inline const char *c_str();
  inline string str() const;

  inline bool set_pos(int64_t pos);

  ~string_buffer() noexcept;

  inline void debug_print() const;

  inline void copy_raw_data(const string_buffer &other);

  friend inline bool operator==(const string_buffer &lhs, const string_buffer &rhs);
  friend inline bool operator!=(const string_buffer &lhs, const string_buffer &rhs);
};

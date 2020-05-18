#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

#define STRING_BUFFER_ERROR_FLAG_ON -1
#define STRING_BUFFER_ERROR_FLAG_OFF 0
#define STRING_BUFFER_ERROR_FLAG_FAILED 1

class string_buffer {
  static dl::size_type MIN_BUFFER_LEN;
  static dl::size_type MAX_BUFFER_LEN;
  char *buffer_end;
  char *buffer_begin;
  dl::size_type buffer_len;

  inline void resize(dl::size_type new_buffer_len);
  inline void reserve_at_least(dl::size_type new_buffer_len);
  string_buffer(const string_buffer &other) = delete;
  string_buffer &operator=(const string_buffer &other) = delete;

public:
  static int string_buffer_error_flag;
  explicit string_buffer(dl::size_type buffer_len = 4000) noexcept;

  inline string_buffer &clean();

  friend inline string_buffer &operator<<(string_buffer &sb, char c);
  friend inline string_buffer &operator<<(string_buffer &sb, const char *s);
  friend inline string_buffer &operator<<(string_buffer &sb, double f);
  friend inline string_buffer &operator<<(string_buffer &sb, const string &s);
  friend inline string_buffer &operator<<(string_buffer &sb, bool x);
  friend inline string_buffer &operator<<(string_buffer &sb, int x);
  friend inline string_buffer &operator<<(string_buffer &sb, unsigned int x);
  friend inline string_buffer &operator<<(string_buffer &sb, long long x);
  friend inline string_buffer &operator<<(string_buffer &sb, unsigned long long x);

  inline string_buffer &append(const char *str, int len);
  inline void write(const char *str, int len);

  inline void append_unsafe(const char *str, int len);

  inline void append_char(char c) __attribute__ ((always_inline));//unsafe

  inline void reserve(int len);

  inline dl::size_type size() const;

  inline char *buffer();
  inline const char *buffer() const;

  inline const char *c_str();
  inline string str() const;

  inline bool set_pos(int pos);

  ~string_buffer() noexcept;

  friend void init_string_buffer_lib(int max_length);

  inline void debug_print() const;

  inline void copy_raw_data(const string_buffer &other);

  friend inline bool operator==(const string_buffer &lhs, const string_buffer &rhs);
  friend inline bool operator!=(const string_buffer &lhs, const string_buffer &rhs);
};

extern string_buffer static_SB;
extern string_buffer static_SB_spare;

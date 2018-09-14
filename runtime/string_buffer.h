#pragma once

/*
 *
 *   Do not include with file directly
 *   Include kphp_core.h instead
 *
 */

#define STRING_BUFFER_ERROR_FLAG_ON -1
#define STRING_BUFFER_ERROR_FLAG_OFF 0
#define STRING_BUFFER_ERROR_FLAG_FAILED 1

class string_buffer {
  static dl::size_type MIN_BUFFER_LEN;
  static dl::size_type MAX_BUFFER_LEN;
  string_buffer(const string_buffer &other);
  string_buffer &operator=(const string_buffer &other);

  char *buffer_end;
  char *buffer_begin;
  dl::size_type buffer_len;

  inline void resize(dl::size_type new_buffer_len);
  inline void reserve_at_least(dl::size_type new_buffer_len);

public:
  static int string_buffer_error_flag;
  inline explicit string_buffer(dl::size_type buffer_len = 4000);

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

  inline void append_unsafe(const char *str, int len);

  inline void append_char(char c) __attribute__ ((always_inline));//unsafe

  inline void reserve(int len);

  inline dl::size_type size() const;

  inline char *buffer();
  inline const char *buffer() const;

  inline const char *c_str();
  inline string str() const;

  inline bool set_pos(int pos);

  inline ~string_buffer();

  friend void string_buffer_init_static(int max_length);
};

string_buffer static_SB __attribute__ ((weak));
string_buffer static_SB_spare __attribute__ ((weak));

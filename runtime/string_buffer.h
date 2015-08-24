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
  static const dl::size_type MIN_BUFFER_LEN = 266175;
  static const dl::size_type MAX_BUFFER_LEN = (1 << 24);
  string_buffer (const string_buffer &other);
  string_buffer &operator = (const string_buffer &other);

  char *buffer_end;
  char *buffer_begin;
  dl::size_type buffer_len;

  inline void resize (dl::size_type new_buffer_len);
  inline void reserve_at_least (dl::size_type new_buffer_len);

public:
  static int string_buffer_error_flag;
  inline explicit string_buffer (dl::size_type buffer_len = 4000);

  inline string_buffer &clean (void);

  inline string_buffer &operator + (char c);
  inline string_buffer &operator + (const char *s);
  inline string_buffer &operator + (double f);
  inline string_buffer &operator + (const string &s);
  inline string_buffer &operator + (bool x);
  inline string_buffer &operator + (int x);
  inline string_buffer &operator + (unsigned int x);
  inline string_buffer &operator + (long long x);
  inline string_buffer &operator + (unsigned long long x);
  inline string_buffer &operator + (const var &v);

  inline string_buffer &append (const char *str, int len);

  inline void append_unsafe (const char *str, int len);

  inline void append_char (char c) __attribute__ ((always_inline));//unsafe

  template <class T>
  inline string_buffer &operator += (const T &s) {
    return *this + s;
  }

  inline void reserve (int len);

  inline dl::size_type size (void) const;

  inline char *buffer (void);
  inline const char *buffer (void) const;

  inline const char *c_str (void);
  inline string str (void) const;

  inline bool set_pos (int pos);

  inline ~string_buffer (void);
};

string_buffer static_SB __attribute__ ((weak));
string_buffer static_SB_spare __attribute__ ((weak));

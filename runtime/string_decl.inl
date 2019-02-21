#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

namespace dl {

inline bool is_hex_digit(const int c) {
  return ((unsigned int)(c - '0') <= 9) | ((unsigned int)((c | 0x20) - 'a') < 6);
}

inline bool is_decimal_digit(const int c) {
  return ((unsigned int)(c - '0') <= 9);
}

}

class string {
public:
  using size_type = dl::size_type;
  static const size_type npos = (size_type)-1;

private:
  char *p;

private:
  struct string_inner {
    size_type size;
    size_type capacity;
    int ref_count;

    inline static string_inner &empty_string();

    inline bool is_shared() const;
    inline void set_length_and_sharable(size_type n);

    inline char *ref_data() const;

    inline static size_type new_capacity(size_type requested_capacity, size_type old_capacity);
    inline static string_inner *create(size_type requested_capacity, size_type old_capacity);

    inline char *reserve(size_type requested_capacity);

    inline void dispose();

    inline void destroy();

    inline char *ref_copy();

    inline char *clone(size_type requested_cap);
  };

  inline string_inner *inner() const;

  inline bool disjunct(const char *s) const;
  inline void set_size(size_type new_size);

  inline static string_inner &empty_string();
  inline static string_inner &single_char_str(char c);

  inline static char *create(const char *beg, const char *end);
  inline static char *create(size_type req, char c);
  inline static char *create(size_type req, bool b);

public:
  static const size_type max_size = ((size_type)-1 - sizeof(string_inner) - 1) / 4;

  inline string();
  inline string(const string &str);
  inline string(string &&str) noexcept;
  inline string(const char *s, size_type n);
  inline explicit string(const char *s);
  inline string(size_type n, char c);
  inline string(size_type n, bool b);
  inline explicit string(int i);
  inline explicit string(double f);

  inline ~string();

  inline string &operator=(const string &str);
  inline string &operator=(string &&str) noexcept;

  inline size_type size() const;

  inline void shrink(size_type n);

  inline size_type capacity() const;

  inline void make_not_shared();

  inline void force_reserve(size_type res);
  inline string &reserve_at_least(size_type res);

  inline bool empty() const;

  inline const char &operator[](size_type pos) const;
  inline char &operator[](size_type pos);

  inline string &append(const string &str) __attribute__ ((always_inline));
  inline string &append(const string &str, size_type pos2, size_type n2) __attribute__ ((always_inline));
  inline string &append(const char *s, size_type n) __attribute__ ((always_inline));
  inline string &append(size_type n, char c) __attribute__ ((always_inline));

  inline string &append(bool b) __attribute__ ((always_inline));
  inline string &append(int i) __attribute__ ((always_inline));
  inline string &append(double d) __attribute__ ((always_inline));
  inline string &append(const var &v) __attribute__ ((always_inline));

  inline string &append_unsafe(bool b) __attribute__((always_inline));
  inline string &append_unsafe(int i) __attribute__((always_inline));
  inline string &append_unsafe(double d) __attribute__((always_inline));
  inline string &append_unsafe(const string &str) __attribute__((always_inline));
  inline string &append_unsafe(const char *s, size_type n) __attribute__((always_inline));
  inline string &append_unsafe(const var &v) __attribute__((always_inline));
  inline string &finish_append() __attribute__((always_inline));

  template<class T>
  inline string &append_unsafe(const array<T> &a) __attribute__((always_inline));

  template<class T>
  inline string &append_unsafe(const OrFalse<T> &v) __attribute__((always_inline));


  inline void push_back(char c);

  inline string &assign(const string &str);
  inline string &assign(const string &str, size_type pos, size_type n);
  inline string &assign(const char *s, size_type n);
  inline string &assign(size_type n, char c);
  inline string &assign(size_type n, bool b);//do not initialize. if b == true - just reserve

  //assign binary string_inner representation
  //can be used only on empty string to receive logically const string
  inline void assign_raw(const char *s);

  inline void swap(string &s);

  inline char *buffer();
  inline const char *c_str() const;

  inline string substr(size_type pos, size_type n) const;

  inline size_type find_first_of(const string &s, size_type pos = 0) const;
  inline size_type find(const string &s, size_type pos = 0) const;

  inline void warn_on_float_conversion() const;

  inline bool try_to_int(int *val) const;
  inline bool try_to_float(double *val) const;

  inline var to_numeric() const;
  inline bool to_bool() const;
  inline static int to_int(const char *s, int l);
  inline int to_int() const;
  inline double to_float() const;
  inline const string &to_string() const;

  inline int safe_to_int() const;

  inline bool is_int() const;
  inline bool is_numeric() const;

  inline int hash() const;

  inline int compare(const string &str) const;

  inline const string get_value(int int_key) const;
  inline const string get_value(const string &string_key) const;
  inline const string get_value(const var &v) const;

  inline int get_reference_counter() const;
  inline void set_reference_counter_to_const();
  inline bool is_const_reference_counter() const;
  inline void set_reference_counter_to_cache();
  inline void destroy_cached();

  inline void destroy() __attribute__((always_inline));

  friend class var;

  template<class T>
  friend class array;
};

inline bool operator==(const string &lhs, const string &rhs);

#define CONST_STRING(const_str) string (const_str, sizeof (const_str) - 1)
#define STRING_EQUALS(str, const_str) (str.size() + 1 == sizeof (const_str) && !strcmp (str.c_str(), const_str))

inline bool operator!=(const string &lhs, const string &rhs);

inline bool is_ok_float(double v);

inline int compare_strings_php_order(const string &lhs, const string &rhs);

inline bool eq2(const string &lhs, const string &rhs);

inline bool neq2(const string &lhs, const string &rhs);

inline void swap(string &lhs, string &rhs);


inline dl::size_type max_string_size(bool) __attribute__((always_inline));
inline dl::size_type max_string_size(int) __attribute__((always_inline));
inline dl::size_type max_string_size(double) __attribute__((always_inline));
inline dl::size_type max_string_size(const string &s) __attribute__((always_inline));
inline dl::size_type max_string_size(const var &v) __attribute__((always_inline));

template<class T>
inline dl::size_type max_string_size(const array<T> &) __attribute__((always_inline));

template<class T>
inline dl::size_type max_string_size(const OrFalse<T> &v) __attribute__((always_inline));

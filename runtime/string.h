#pragma once

/*
 *
 *   Do not include with file directly
 *   Include kphp_core.h instead
 *
 */

namespace dl {

inline bool is_hex_digit (const int c) {
  return ((unsigned int)(c - '0') <= 9) | ((unsigned int)((c | 0x20) - 'a') < 6);
}

inline bool is_decimal_digit (const int c) {
  return ((unsigned int)(c - '0') <= 9);
}

}

class string {
public:
  typedef dl::size_type size_type;
  static const size_type npos = (size_type) -1;

private:
  struct string_inner {
    size_type size;
    size_type capacity;
    int ref_count;

    static size_type empty_string_storage[];

    inline static string_inner& empty_string (void);

    inline bool is_shared (void) const;
    inline void set_length_and_sharable (size_type n);
    inline void set_length_and_sharable_force (size_type n);

    inline char *ref_data (void) const;

    inline static size_type new_capacity (size_type requested_capacity, size_type old_capacity);
    inline static string_inner *create (size_type requested_capacity, size_type old_capacity);

    inline char *reserve (size_type requested_capacity);

    inline void dispose (void);

    inline void destroy (void);

    inline char *ref_copy (void);

    inline char *clone (size_type requested_cap);
  };

  char *p;

  inline string_inner *inner (void) const;

  inline bool disjunct (const char *s) const;
  inline void set_size (size_type new_size);

  inline static string_inner& empty_string (void);

  inline static char *create (const char *beg, const char *end);
  inline static char *create (size_type req, char c);
  inline static char *create (size_type req, bool b);

public:
  static const size_type max_size = ((size_type)-1 - sizeof (string_inner) - 1) / 4;

  inline string (void);
  inline string (const string &str);
  inline string (const char *s, size_type n);
  inline string (size_type n, char c);
  inline string (size_type n, bool b);
  inline explicit string (int i);
  inline explicit string (double f);

  inline ~string (void);

  inline string& operator = (const string &str);

  inline size_type size (void) const;

  inline void shrink (size_type n);

  inline size_type capacity (void) const;

  inline void make_not_shared (void);

  inline void force_reserve (size_type res);
  inline string& reserve_at_least (size_type res);

  inline bool empty (void) const;

  inline const char& operator[] (size_type pos) const;
  inline char& operator[] (size_type pos);

  inline string& append (const string &str) __attribute__ ((always_inline));
  inline string& append (const string &str, size_type pos2, size_type n2) __attribute__ ((always_inline));
  inline string& append (const char *s, size_type n) __attribute__ ((always_inline));
  inline string& append (size_type n, char c) __attribute__ ((always_inline));

  inline string& append (bool b) __attribute__ ((always_inline));
  inline string& append (int i) __attribute__ ((always_inline));
  inline string& append (double d) __attribute__ ((always_inline));
  inline string& append (const var &v) __attribute__ ((always_inline));

  inline string& append_unsafe (bool b) __attribute__((always_inline));
  inline string& append_unsafe (int i) __attribute__((always_inline));
  inline string& append_unsafe (double d) __attribute__((always_inline));
  inline string& append_unsafe (const string &str) __attribute__((always_inline));
  inline string& append_unsafe (const char *s, size_type n) __attribute__((always_inline));
  inline string& append_unsafe (const var &v) __attribute__((always_inline));
  inline string& finish_append (void) __attribute__((always_inline));

  template <class T, class TT>
  inline string& append_unsafe (const array <T, TT> &a) __attribute__((always_inline));

  template <class T>
  inline string& append_unsafe (const OrFalse <T> &v) __attribute__((always_inline));


  inline void push_back (char c);

  inline string& assign (const string &str);
  inline string& assign (const string &str, size_type pos, size_type n);
  inline string& assign (const char *s, size_type n);
  inline string& assign (size_type n, char c);
  inline string& assign (size_type n, bool b);//do not initialize. if b == true - just reserve

  //assign binary string_inner representation
  //can be used only on empty string to receive logically const string
  inline void assign_raw (const char *s);

  inline void swap (string &s);

  inline char *buffer (void);
  inline const char *c_str (void) const;

  inline string substr (size_type pos, size_type n) const;

  inline size_type find_first_of (string const &s, size_type pos = 0) const;
  inline size_type find (string const &s, size_type pos = 0) const;

  inline void warn_on_float_conversion (void) const;

  inline bool try_to_int (int *val) const;
  inline bool try_to_float (double *val) const;

  inline var to_numeric (void) const;
  inline bool to_bool (void) const;
  inline int to_int (void) const;
  inline double to_float (void) const;
  inline const string &to_string (void) const;

  inline int safe_to_int (void) const;

  inline bool is_int (void) const;
  inline bool is_numeric (void) const;

  inline int hash (void) const;

  inline int compare (const string& str) const;

  inline const string get_value (int int_key) const;
  inline const string get_value (const string &string_key) const;
  inline const string get_value (const var &v) const;

  inline int get_reference_counter (void) const;

  friend class var;

  template <class T, class TT>
  friend class array;
};

inline bool operator == (const string &lhs, const string &rhs);

#define CONST_STRING(const_str) string (const_str, sizeof (const_str) - 1)
#define STRING_EQUALS(str, const_str) (str.size() + 1 == sizeof (const_str) && !strcmp (str.c_str(), const_str))

inline bool operator != (const string &lhs, const string &rhs);

inline bool is_ok_float (double v);

inline bool eq2 (const string &lhs, const string &rhs);

inline bool neq2 (const string &lhs, const string &rhs);

inline void swap (string &lhs, string &rhs);


inline dl::size_type max_string_size (bool) __attribute__((always_inline));
inline dl::size_type max_string_size (int) __attribute__((always_inline));
inline dl::size_type max_string_size (double) __attribute__((always_inline));
inline dl::size_type max_string_size (const string &s) __attribute__((always_inline));
inline dl::size_type max_string_size (const var &v) __attribute__((always_inline));

template <class T, class TT>
inline dl::size_type max_string_size (const array <T, TT> &) __attribute__((always_inline));

template <class T>
inline dl::size_type max_string_size (const OrFalse <T> &v) __attribute__((always_inline));

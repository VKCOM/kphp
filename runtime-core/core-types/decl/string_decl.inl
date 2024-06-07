#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from runtime-core.h"
#endif

using string_size_type = uint32_t;

inline bool string_to_bool(const char *s, string_size_type size) {
  return size >= 2 || (size == 1 && s[0] != '0');
}

// tmp_string is not a real string: it can't be used in a place where string types are expected;
// it can be a result of compiler-inserted evaluation that should be then recognized by the
// runtime functions overloading
//
// in a sense, it's like a string view, but only valid for a single expression evaluation
struct tmp_string {
  const char *data = nullptr;
  string_size_type size = 0;

  tmp_string() = default;
  inline tmp_string(const char *data, string_size_type size);
  explicit inline tmp_string(const string &s);

  bool to_bool() const noexcept {
    return data && string_to_bool(data, size);
  }

  bool has_value() const noexcept { return data != nullptr; }
  bool empty() const noexcept { return size == 0; }
};

struct ArrayBucketDummyStrTag;

class string {
public:
  using size_type = string_size_type;
  static constexpr size_type npos = (size_type)-1;

private:
  char *p;

private:
  struct string_inner {
    size_type size;
    size_type capacity;
    int ref_count;

    inline bool is_shared() const;
    inline void set_length_and_sharable(size_type n);

    inline char *ref_data() const;

    inline static size_type new_capacity(size_type requested_capacity, size_type old_capacity);
    inline static string_inner *create(size_type requested_capacity, size_type old_capacity);

    inline char *reserve(size_type requested_capacity);

    inline void dispose();

    inline void destroy();

    inline size_type get_memory_usage() const;

    inline char *ref_copy();

    inline char *clone(size_type requested_cap);
  };

  inline string_inner *inner() const;

  inline bool disjunct(const char *s) const;
  inline void set_size(size_type new_size);

  inline static char *create(const char *beg, const char *end);
  // IMPORTANT: this function may return read-only strings for n == 0 and n == 1.
  // Use it unless you have to manually write something into the buffer.
  inline static char *create(size_type req, char c);
  inline static char *create(size_type req, bool b);

  friend class string_cache;

public:
  static constexpr size_type max_size() noexcept {
    return ((size_type)-1 - sizeof(string_inner) - 1) / 4;
  }

  static size_type unsafe_cast_to_size_type(int64_t size) noexcept {
    php_assert(size >= 0);
    if (unlikely(size > max_size())) {
      php_critical_error ("Trying to make too big string of size %" PRId64, size);
    }
    return static_cast<size_type>(size);
  }

  inline string();
  inline string(const string &str) noexcept;
  inline string(string &&str) noexcept;
  inline string(const char *s, size_type n);
  inline explicit string(const char *s);
  // IMPORTANT: this constructor may return read-only strings for n == 0 and n == 1.
  // Use it unless you have to manually operate with string's internal buffer.
  inline string(size_type n, char c);
  inline string(size_type n, bool b);
  inline explicit string(int64_t i);
  inline explicit string(int32_t i): string(static_cast<int64_t>(i)) {}
  inline explicit string(double f);

  ~string() noexcept;

  // Achtung! Do not use it!
  // This constructor intended ONLY for usage inside array's bucket
  // in order to being able to distinguish which key type is stored there
  inline explicit string(ArrayBucketDummyStrTag) noexcept;
  inline bool is_dummy_string() const noexcept;

  inline string &operator=(const string &str) noexcept;
  inline string &operator=(string &&str) noexcept;

  inline size_type size() const;

  inline void shrink(size_type n);

  inline size_type capacity() const;

  inline void make_not_shared();

  inline string copy_and_make_not_shared() const;

  inline void force_reserve(size_type res);
  inline string &reserve_at_least(size_type res);

  inline bool empty() const;
  inline bool starts_with(const string &other) const noexcept;
  inline bool ends_with(const string &other) const noexcept;

  inline const char &operator[](size_type pos) const;
  inline char &operator[](size_type pos);

  inline string &append(const string &str) __attribute__ ((always_inline));
  inline string &append(const string &str, size_type pos2, size_type n2) __attribute__ ((always_inline));
  inline string &append(const char *s) __attribute__ ((always_inline));
  inline string &append(const char *s, size_type n) __attribute__ ((always_inline));
  inline string &append(size_type n, char c) __attribute__ ((always_inline));

  inline string &append(bool b) __attribute__ ((always_inline));
  inline string &append(int64_t i) __attribute__ ((always_inline));
  inline string &append(int32_t v) {return append(int64_t{v});}
  inline string &append(double d) __attribute__ ((always_inline));
  inline string &append(const mixed &v) __attribute__ ((always_inline));

  inline string &append_unsafe(bool b) __attribute__((always_inline));
  inline string &append_unsafe(int64_t i) __attribute__((always_inline));
  inline string &append_unsafe(int32_t v) {return append(int64_t{v});}
  inline string &append_unsafe(double d) __attribute__((always_inline));
  inline string &append_unsafe(const string &str) __attribute__((always_inline));
  inline string &append_unsafe(tmp_string str) __attribute__((always_inline));
  inline string &append_unsafe(const char *s, size_type n) __attribute__((always_inline));
  inline string &append_unsafe(const mixed &v) __attribute__((always_inline));
  inline string &finish_append() __attribute__((always_inline));

  template<class T>
  inline string &append_unsafe(const array<T> &a) __attribute__((always_inline));

  template<class T>
  inline string &append_unsafe(const Optional<T> &v) __attribute__((always_inline));


  inline void push_back(char c);

  inline string &assign(const string &str);
  inline string &assign(const string &str, size_type pos, size_type n);
  inline string &assign(const char *s);
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

  inline bool try_to_int(int64_t *val) const;
  inline bool try_to_float(double *val, bool php8_warning = true) const;
  inline bool try_to_float_as_php7(double *val) const;
  inline bool try_to_float_as_php8(double *val) const;

  inline mixed to_numeric() const;
  inline bool to_bool() const;
  inline static int64_t to_int(const char *s, size_type l);
  inline int64_t to_int(int64_t start, int64_t l) const;
  inline int64_t to_int() const;
  inline double to_float() const;
  inline const string &to_string() const;

  inline int64_t safe_to_int() const;

  inline bool is_int() const;
  inline bool is_numeric_as_php7() const;
  inline bool is_numeric_as_php8() const;
  inline bool is_numeric() const;

  inline int64_t hash() const;

  static inline int64_t compare(const string &str, const char *other, size_type other_size);
  inline int64_t compare(const string &str) const;

  inline bool isset(int64_t index) const;
  static inline int64_t get_correct_offset(size_type size, int64_t offset);
  inline int64_t get_correct_index(int64_t index) const;
  inline int64_t get_correct_offset(int64_t offset) const;
  inline int64_t get_correct_offset_clamped(int64_t offset) const;
  inline const string get_value(int64_t int_key) const;
  inline const string get_value(const string &string_key) const;
  inline const string get_value(const mixed &v) const;

  inline int64_t get_reference_counter() const;

  inline bool is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept;
  inline void set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept;
  inline void force_destroy(ExtraRefCnt expected_ref_cnt) noexcept;

  inline size_type estimate_memory_usage() const;
  inline static size_type estimate_memory_usage(size_t len) noexcept;

  inline static constexpr size_t inner_sizeof() noexcept { return sizeof(string_inner); }
  inline static string make_const_string_on_memory(const char *str, size_type len, void *memory, size_t memory_size);

  inline void destroy() __attribute__((always_inline));
};

inline string materialize_tmp_string(tmp_string s) {
  if (!s.data) {
    return string{}; // an empty string
  }
  return string{s.data, s.size};
}

inline bool operator==(const string &lhs, const string &rhs);

#define CONST_STRING(const_str) string (const_str, sizeof (const_str) - 1)
#define STRING_EQUALS(str, const_str) (str.size() + 1 == sizeof (const_str) && !strcmp (str.c_str(), const_str))

inline bool operator!=(const string &lhs, const string &rhs);

inline bool is_ok_float(double v);

inline int64_t compare_strings_php_order(const string &lhs, const string &rhs);

inline void swap(string &lhs, string &rhs);


inline string::size_type max_string_size(bool) __attribute__((always_inline));
inline string::size_type max_string_size(int64_t) __attribute__((always_inline));
inline string::size_type max_string_size(int32_t) __attribute__((always_inline));
inline string::size_type max_string_size(double) __attribute__((always_inline));
inline string::size_type max_string_size(const string &s) __attribute__((always_inline));
inline string::size_type max_string_size(tmp_string s) __attribute__((always_inline));
inline string::size_type max_string_size(const mixed &v) __attribute__((always_inline));

template<class T>
inline string::size_type max_string_size(const array<T> &) __attribute__((always_inline));

template<class T>
inline string::size_type max_string_size(const Optional<T> &v) __attribute__((always_inline));

inline bool wrap_substr_args(string::size_type str_len, int64_t &start, int64_t &length) {
  if (length < 0 && -length > str_len) {
    return false;
  }

  if (length > str_len) {
    length = str_len;
  }

  if (start > str_len) {
    return false;
  }

  if (length < 0 && length < start - str_len) {
    return false;
  }
  start = string::get_correct_offset(str_len, start);
  if (length < 0) {
    length = (str_len - start) + length;
    if (length < 0) {
      length = 0;
    }
  }

  if (length > str_len - start) {
    length = str_len - start;
  }

  return true;
}

#pragma once

#include "runtime/array_iterator.h"
#include "runtime/include.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

struct array_size {
  int int_size;
  int string_size;
  bool is_vector;

  inline array_size(int int_size, int string_size, bool is_vector);

  inline array_size operator+(const array_size &other) const;

  inline array_size &cut(int length);

  inline array_size &min(const array_size &other);
};

namespace dl {
template<class T, class TT, class T1>
void sort(TT *begin_init, TT *end_init, const T1 &compare);
}

enum class overwrite_element {
  YES,
  NO
};

template<class T>
class array {

public:
  using ValueType = T;

  typedef var key_type;
  typedef T value_type;

  inline static bool is_int_key(const key_type &key);

private:

  using entry_pointer_type = dl::size_type;

  struct list_hash_entry {
    entry_pointer_type next;
    entry_pointer_type prev;
  };

  struct int_hash_entry : list_hash_entry {
    T value;

    int int_key;

    inline key_type get_key() const;
  };

  struct string_hash_entry : list_hash_entry {
    T value;

    int int_key;
    string string_key;

    inline key_type get_key() const;
  };

  // `max_key` and `string_size` could be also be there
  // but sometimes, for simplicity, we use them in vector too
  // to not add extra checks they are left in `array_inner`
  struct array_inner_fields_for_map {
    uint64_t modulo_helper_int_buf_size{0};
    uint64_t modulo_helper_string_buf_size{0};
  };

  struct array_inner {
    //if key is number, int_key contains this number, there is no string_key.
    //if key is string, int_key contains hash of this string, string_key contains this string.
    //empty hash_entry identified by (next == EMPTY_POINTER)
    //vector is_identified by string_buf_size == -1

    static const int MAX_HASHTABLE_SIZE = (1 << 26);
    static const int MIN_HASHTABLE_SIZE = 1;
    static const int DEFAULT_HASHTABLE_SIZE = (1 << 3);

    static const entry_pointer_type EMPTY_POINTER;
    static const T empty_T;

    int ref_cnt;
    int max_key;
    list_hash_entry end_;
    int int_size;
    int int_buf_size;
    int string_size;
    int string_buf_size;
    int_hash_entry int_entries[0];

    inline bool is_vector() const __attribute__ ((always_inline));

    inline list_hash_entry *get_entry(entry_pointer_type pointer) const __attribute__ ((always_inline));
    inline entry_pointer_type get_pointer(list_hash_entry *entry) const __attribute__ ((always_inline));

    inline const string_hash_entry *begin() const __attribute__ ((always_inline));
    inline const string_hash_entry *next(const string_hash_entry *p) const __attribute__ ((always_inline));
    inline const string_hash_entry *prev(const string_hash_entry *p) const __attribute__ ((always_inline));
    inline const string_hash_entry *end() const __attribute__ ((always_inline));

    inline string_hash_entry *begin() __attribute__ ((always_inline));
    inline string_hash_entry *next(string_hash_entry *p) __attribute__ ((always_inline));
    inline string_hash_entry *prev(string_hash_entry *p) __attribute__ ((always_inline));
    inline string_hash_entry *end() __attribute__ ((always_inline));

    inline bool is_string_hash_entry(const string_hash_entry *p) const __attribute__ ((always_inline));
    inline const string_hash_entry *get_string_entries() const __attribute__ ((always_inline));
    inline string_hash_entry *get_string_entries() __attribute__ ((always_inline));

    inline array_inner_fields_for_map &fields_for_map() __attribute__((always_inline));
    inline const array_inner_fields_for_map &fields_for_map() const __attribute__((always_inline));

    inline int choose_bucket_int(int key) const __attribute__ ((always_inline));
    inline int choose_bucket_string(int key) const __attribute__ ((always_inline));
    inline static int choose_bucket(const int key, const int buf_size, const uint64_t modulo_helper) __attribute__ ((always_inline));

    inline static dl::size_type sizeof_vector(int int_size) __attribute__((always_inline));
    inline static dl::size_type sizeof_map(int int_size, int string_size) __attribute__((always_inline));
    inline static dl::size_type estimate_size(int &new_int_size, int &new_string_size, bool is_vector);
    inline static array_inner *create(int new_int_size, int new_string_size, bool is_vector);

    inline static array_inner *empty_array() __attribute__ ((always_inline));

    inline void dispose() __attribute__ ((always_inline));

    inline array_inner *ref_copy() __attribute__ ((always_inline));


    template<class ...Args>
    inline T &emplace_back_vector_value(Args &&... args) noexcept;
    inline T &push_back_vector_value(const T &v); //unsafe

    template<class ...Args>
    inline T &emplace_vector_value(int int_key, Args &&... args) noexcept;
    inline T &set_vector_value(int int_key, const T &v); //unsafe

    template<class ...Args>
    inline T &emplace_int_key_map_value(overwrite_element policy, int int_key, Args &&... args) noexcept;
    inline T &set_map_value(overwrite_element policy, int int_key, const T &v);

    template<class STRING, class ...Args>
    inline T &emplace_string_key_map_value(overwrite_element policy, int int_key, STRING &&string_key, Args &&... args) noexcept;
    inline T &set_map_value(overwrite_element policy, int int_key, const string &string_key, const T &v);

    inline void unset_vector_value();
    inline void unset_map_value(int int_key);

    inline const T *find_value(int int_key) const;
    inline const T *find_value(const string &string_key, int precomuted_hash) const;

    inline const T &get_vector_value(int int_key) const;//unsafe
    inline T &get_vector_value(int int_key);//unsafe
    inline void unset_map_value(int int_key, const string &string_key);

    dl::size_type estimate_memory_usage() const;

    inline array_inner() = delete;
    inline array_inner(const array_inner &other) = delete;
    inline array_inner &operator=(const array_inner &other) = delete;
  };

  inline bool mutate_if_vector_shared(int mul = 1);
  inline bool mutate_to_size_if_vector_shared(int int_size);
  inline void mutate_to_size(int int_size);
  inline bool mutate_if_map_shared(int mul = 1);
  inline void mutate_if_vector_needed_int();
  inline void mutate_if_map_needed_int();
  inline void mutate_if_map_needed_string();
  inline void mutate_to_map_if_vector_or_map_need_string();

  inline void convert_to_map();

  template<class T1>
  inline void copy_from(const array<T1> &other);

  template<class T1>
  inline void move_from(array<T1> &&other) noexcept;

  inline void destroy() __attribute__ ((always_inline));

public:
  friend class array_iterator<T>;
  friend class array_iterator<const T>;

  using iterator = array_iterator<T>;
  using const_iterator = array_iterator<const T>;

  inline array() __attribute__ ((always_inline));

  inline explicit array(const array_size &s) __attribute__ ((always_inline));

  template<class KeyT>
  inline array(const std::initializer_list<std::pair<KeyT, T>> &list) __attribute__ ((always_inline));

  inline array(const array &other) __attribute__ ((always_inline));

  inline array(array &&other) noexcept __attribute__ ((always_inline));

  template<class T1, class = enable_if_constructible_or_unknown<T, T1>>
  inline array(const array<T1> &other) __attribute__ ((always_inline));

  template<class T1, class = enable_if_constructible_or_unknown<T, T1>>
  inline array(array<T1> &&other) noexcept __attribute__ ((always_inline));

  template<class... Args>
  inline static array create(Args &&... args) __attribute__ ((always_inline));

  inline array &operator=(const array &other) __attribute__ ((always_inline));

  inline array &operator=(array &&other) noexcept __attribute__ ((always_inline));

  template<class T1, class = enable_if_constructible_or_unknown<T, T1>>
  inline array &operator=(const array<T1> &other) __attribute__ ((always_inline));

  template<class T1, class = enable_if_constructible_or_unknown<T, T1>>
  inline array &operator=(array<T1> &&other) noexcept;

  inline ~array() /*__attribute__ ((always_inline))*/;

  inline void clear() __attribute__ ((always_inline));

  inline bool is_vector() const __attribute__ ((always_inline));

  T &operator[](int int_key);
  T &operator[](const string &s);
  T &operator[](const var &v);
  T &operator[](const const_iterator &it);
  T &operator[](const iterator &it);

  template<class ...Args>
  void emplace_value(int int_key, Args &&... args) noexcept;
  void set_value(int int_key, T &&v) noexcept;
  void set_value(int int_key, const T &v) noexcept;

  template<class ...Args>
  void emplace_value(const string &string_key, Args &&... args) noexcept;
  void set_value(const string &string_key, T &&v) noexcept;
  void set_value(const string &string_key, const T &v) noexcept;

  void set_value(const string &string_key, T &&v, int precomuted_hash) noexcept;
  void set_value(const string &string_key, const T &v, int precomuted_hash) noexcept;

  template<class ...Args>
  void emplace_value(const var &var_key, Args &&... args) noexcept;
  void set_value(const var &v, T &&value) noexcept;
  void set_value(const var &v, const T &value) noexcept;


  template<class OptionalT, class ...Args>
  void emplace_value(const Optional<OptionalT> &key, Args &&... args) noexcept;
  template<class OptionalT>
  void set_value(const Optional<OptionalT> &key, T &&value) noexcept;
  template<class OptionalT>
  void set_value(const Optional<OptionalT> &key, const T &value) noexcept;

  void set_value(const const_iterator &it);
  void set_value(const iterator &it);

  // assign binary array_inner representation
  // can be used only on empty arrays to receive logically const array
  void assign_raw(const char *s);

  const T *find_value(int int_key) const;
  const T *find_value(const string &s) const;
  const T *find_value(const string &s, int precomuted_hash) const;
  const T *find_value(const var &v) const;
  const T *find_value(const const_iterator &it) const;
  const T *find_value(const iterator &it) const;

  template<class K>
  const var get_var(const K &key) const;

  template<class K>
  const T get_value(const K &key) const;
  const T get_value(const string &string_key, int precomuted_hash) const;

  template<class ...Args>
  T &emplace_back(Args &&... args) noexcept;
  void push_back(T &&v) noexcept;
  void push_back(const T &v) noexcept;

  void push_back(const const_iterator &it);
  void push_back(const iterator &it);

  const T push_back_return(const T &v);
  const T push_back_return(T &&v);

  inline void fill_vector(int num, const T &value);
  inline void memcpy_vector(int num, const void *src_buf);

  inline int get_next_key() const __attribute__ ((always_inline));

  template<class K>
  bool has_key(const K &key) const;

  template<class K>
  bool isset(const K &key) const;

  void unset(int int_key);
  void unset(const string &string_key);
  void unset(const var &var_key);

  inline bool empty() const __attribute__ ((always_inline));
  inline int count() const __attribute__ ((always_inline));

  inline array_size size() const __attribute__ ((always_inline));

  template<class T1, class = enable_if_constructible_or_unknown<T, T1>>
  void merge_with(const array<T1> &other);

  const array operator+(const array &other) const;
  array &operator+=(const array &other);

  inline const_iterator begin() const __attribute__ ((always_inline));
  inline const_iterator cbegin() const __attribute__ ((always_inline));
  inline const_iterator middle(int n) const __attribute__ ((always_inline));
  inline const_iterator end() const __attribute__ ((always_inline));
  inline const_iterator cend() const __attribute__ ((always_inline));

  inline iterator begin() __attribute__ ((always_inline));
  inline iterator middle(int n) __attribute__ ((always_inline));
  inline iterator end() __attribute__ ((always_inline));

  template<class T1>
  void sort(const T1 &compare, bool renumber);

  template<class T1>
  void ksort(const T1 &compare);

  inline void swap(array &other) __attribute__ ((always_inline));


  T pop();

  T shift();

  int unshift(const T &val);


  inline bool to_bool() const __attribute__ ((always_inline));
  inline int to_int() const __attribute__ ((always_inline));
  inline double to_float() const __attribute__ ((always_inline));


  int get_reference_counter() const;
  void set_reference_counter_to_const();
  bool is_const_reference_counter() const;

  iterator begin_no_mutate();
  iterator end_no_mutate();
  void set_reference_counter_to_cache();
  bool is_cache_reference_counter() const;
  void destroy_cached();

  const T *get_const_vector_pointer() const; // unsafe

  void reserve(int int_size, int string_size, bool make_vector_if_possible);

  dl::size_type estimate_memory_usage() const;

  template<typename U>
  static array<T> convert_from(const array<U> &);
private:
  void push_back_values() {}

  template<class Arg, class...Args>
  void push_back_values(Arg &&arg, Args &&... args);

private:
  array_inner *p;

  template<class T1>
  friend class array;
};

template<class T>
inline void swap(array<T> &lhs, array<T> &rhs);

template<class T>
inline const array<T> array_add(array<T> a1, const array<T> &a2);


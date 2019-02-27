#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

array_size::array_size(int int_size, int string_size, bool is_vector) :
  int_size(int_size),
  string_size(string_size),
  is_vector(is_vector) {}

array_size array_size::operator+(const array_size &other) const {
  return array_size(int_size + other.int_size, string_size + other.string_size, is_vector && other.is_vector);
}

array_size &array_size::cut(int length) {
  if (int_size > length) {
    int_size = length;
  }
  if (string_size > length) {
    string_size = length;
  }
  return *this;
}

array_size &array_size::min(const array_size &other) {
  if (int_size > other.int_size) {
    int_size = other.int_size;
  }

  if (string_size > other.string_size) {
    string_size = other.string_size;
  }

  is_vector &= other.is_vector;
  return *this;
}

namespace dl {

template<class T, class T1>
void sort(T *begin_init, T *end_init, const T1 &compare) {
  T *begin_stack[32];
  T *end_stack[32];

  begin_stack[0] = begin_init;
  end_stack[0] = end_init - 1;

  for (int depth = 0; depth >= 0; --depth) {
    T *begin = begin_stack[depth];
    T *end = end_stack[depth];

    while (begin < end) {
      int offset = (end - begin) >> 1;
      swap(*begin, begin[offset]);

      T *i = begin + 1, *j = end;

      while (1) {
        while (i < j && compare(*begin, *i) > 0) {
          i++;
        }

        while (i <= j && compare(*j, *begin) > 0) {
          j--;
        }

        if (i >= j) {
          break;
        }

        swap(*i++, *j--);
      }

      swap(*begin, *j);

      if (j - begin <= end - j) {
        if (j + 1 < end) {
          begin_stack[depth] = j + 1;
          end_stack[depth++] = end;
        }
        end = j - 1;
      } else {
        if (begin < j - 1) {
          begin_stack[depth] = begin;
          end_stack[depth++] = j - 1;
        }
        begin = j + 1;
      }
    }
  }
}

} // namespace dl

template<class T>
typename array<T>::key_type array<T>::int_hash_entry::get_key() const {
  return key_type(int_key);
}

template<class T>
typename array<T>::key_type array<T>::string_hash_entry::get_key() const {
  return key_type(string_key);
}

template<class T>
bool array<T>::is_int_key(const typename array<T>::key_type &key) {
  return key.is_int();
}

template<>
inline typename array<Unknown>::array_inner *array<Unknown>::array_inner::empty_array() {
  static array_inner empty_array = {
    REF_CNT_FOR_CONST /* ref_cnt */, -1 /* max_key */,
    0 /* end_.next */, 0 /* end_.prev */,
    0 /* int_size */, 2 /* int_buf_size */,
    0 /* string_size */, -1 /* string_buf_size */,
    {} /* int_entries */
  };
  return &empty_array;
}

template<class T>
typename array<T>::array_inner *array<T>::array_inner::empty_array() {
  return reinterpret_cast<array_inner *>(array<Unknown>::array_inner::empty_array());
}

template<class T>
int array<T>::array_inner::choose_bucket(const int key, const int buf_size) {
  return (unsigned int)(key << 2) /* 2654435761u */ % buf_size;
}

template<class T>
const typename array<T>::entry_pointer_type array<T>::array_inner::EMPTY_POINTER = entry_pointer_type();

template<class T>
const T array<T>::array_inner::empty_T = T();


template<class T>
bool array<T>::array_inner::is_vector() const {
  return string_buf_size == -1;
}


template<class T>
typename array<T>::list_hash_entry *array<T>::array_inner::get_entry(entry_pointer_type pointer) const {
  return (list_hash_entry * )((char *)this + pointer);
//  return pointer;
}

template<class T>
typename array<T>::entry_pointer_type array<T>::array_inner::get_pointer(list_hash_entry *entry) const {
  return (entry_pointer_type)((char *)entry - (char *)this);
//  return entry;
}


template<class T>
const typename array<T>::string_hash_entry *array<T>::array_inner::begin() const {
  return (const string_hash_entry *)get_entry(end_.next);
}

template<class T>
const typename array<T>::string_hash_entry *array<T>::array_inner::next(const string_hash_entry *p) const {
  return (const string_hash_entry *)get_entry(p->next);
}

template<class T>
const typename array<T>::string_hash_entry *array<T>::array_inner::prev(const string_hash_entry *p) const {
  return (const string_hash_entry *)get_entry(p->prev);
}

template<class T>
const typename array<T>::string_hash_entry *array<T>::array_inner::end() const {
  return (const string_hash_entry *)&end_;
}

template<class T>
typename array<T>::string_hash_entry *array<T>::array_inner::begin() {
  return (string_hash_entry *)get_entry(end_.next);
}

template<class T>
typename array<T>::string_hash_entry *array<T>::array_inner::next(string_hash_entry *p) {
  return (string_hash_entry *)get_entry(p->next);
}

template<class T>
typename array<T>::string_hash_entry *array<T>::array_inner::prev(string_hash_entry *p) {
  return (string_hash_entry *)get_entry(p->prev);
}

template<class T>
typename array<T>::string_hash_entry *array<T>::array_inner::end() {
  return (string_hash_entry * ) & end_;
}

template<class T>
bool array<T>::array_inner::is_string_hash_entry(const string_hash_entry *p) const {
  return p >= get_string_entries();
}

template<class T>
const typename array<T>::string_hash_entry *array<T>::array_inner::get_string_entries() const {
  return (const string_hash_entry *)(int_entries + int_buf_size);
}

template<class T>
typename array<T>::string_hash_entry *array<T>::array_inner::get_string_entries() {
  return (string_hash_entry * )(int_entries + int_buf_size);
}


template<class T>
typename array<T>::array_inner *array<T>::array_inner::create(int new_int_size, int new_string_size, bool is_vector) {
  if (new_int_size + new_string_size > MAX_HASHTABLE_SIZE) {
    php_critical_error ("max array size exceeded");
  }

  if (new_int_size < 0) {
    new_int_size = 0;
  }
  if (new_string_size < 0) {
    new_string_size = 0;
  }
  if (new_int_size + new_string_size < MIN_HASHTABLE_SIZE) {
//    new_int_size = MIN_HASHTABLE_SIZE;
  }

  if (is_vector) {
    php_assert (new_string_size == 0);
    new_int_size += 2;

    array_inner *p = (array_inner *)dl::allocate((dl::size_type)(sizeof(array_inner) + new_int_size * sizeof(T)));
    p->ref_cnt = 0;
    p->max_key = -1;
    p->int_size = 0;
    p->int_buf_size = new_int_size;
    p->string_size = 0;
    p->string_buf_size = -1;
    return p;
  }

  new_int_size = 2 * new_int_size + 3;
  if (new_int_size % 5 == 0) {
    new_int_size += 2;
  }
  new_string_size = 2 * new_string_size + 3;
  if (new_string_size % 5 == 0) {
    new_string_size += 2;
  }

  array_inner *p = (array_inner *)dl::allocate0((dl::size_type)(sizeof(array_inner) + new_int_size * sizeof(int_hash_entry) + new_string_size * sizeof(string_hash_entry)));
  p->ref_cnt = 0;
  p->max_key = -1;
  p->end()->next = p->get_pointer(p->end());
  p->end()->prev = p->get_pointer(p->end());
  p->int_buf_size = new_int_size;
  p->string_buf_size = new_string_size;
  return p;
}

template<class T>
void array<T>::array_inner::dispose() {
  if (ref_cnt != REF_CNT_FOR_CONST) {
    ref_cnt--;
    if (ref_cnt <= -1) {
      if (is_vector()) {
        for (int i = 0; i < int_size; i++) {
          ((T *)int_entries)[i].~T();
        }

        dl::deallocate((void *)this, (dl::size_type)(sizeof(array_inner) + int_buf_size * sizeof(T)));
        return;
      }

      for (const string_hash_entry *it = begin(); it != end(); it = next(it)) {
        it->value.~T();
        if (is_string_hash_entry(it)) {
          it->string_key.~string();
        }
      }

      dl::deallocate((void *)this, (dl::size_type)(sizeof(array_inner) + int_buf_size * sizeof(int_hash_entry) + string_buf_size * sizeof(string_hash_entry)));
    }
  }
}


template<class T>
typename array<T>::array_inner *array<T>::array_inner::ref_copy() {
  if (ref_cnt != REF_CNT_FOR_CONST) {
    ref_cnt++;
  }
  return this;
}


template<class T>
const var array<T>::array_inner::get_var(int int_key) const {
  if (is_vector()) {
    if ((unsigned int)int_key < (unsigned int)int_size) {
      return get_vector_value(int_key);
    }

    return var();
  }

  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  if (int_entries[bucket].next == EMPTY_POINTER) {
    return var();
  }

  return int_entries[bucket].value;
}


template<class T>
const T array<T>::array_inner::get_value(int int_key) const {
  if (is_vector()) {
    if ((unsigned int)int_key < (unsigned int)int_size) {
      return get_vector_value(int_key);
    }

    return empty_T;
  }

  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  if (int_entries[bucket].next == EMPTY_POINTER) {
    return empty_T;
  }

  return int_entries[bucket].value;
}


template<class T>
T &array<T>::array_inner::push_back_vector_value(const T &v) {
  php_assert (int_size < int_buf_size);
  new(&((T *)int_entries)[int_size]) T(v);
  max_key++;
  int_size++;

  return reinterpret_cast<T *>(int_entries)[max_key];
}

template<class T>
T &array<T>::array_inner::get_vector_value(int int_key) {
  return reinterpret_cast<T *>(int_entries)[int_key];
}

template<class T>
const T &array<T>::array_inner::get_vector_value(int int_key) const {
  return reinterpret_cast<const T *>(int_entries)[int_key];
}

template<class T>
T &array<T>::array_inner::set_vector_value(int int_key, const T &v) {
  reinterpret_cast<T *>(int_entries)[int_key] = v;
  return get_vector_value(int_key);
}


template<class T>
T &array<T>::array_inner::set_map_value(int int_key, const T &v, bool save_value) {
  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  if (int_entries[bucket].next == EMPTY_POINTER) {
    int_entries[bucket].int_key = int_key;

    int_entries[bucket].prev = end()->prev;
    get_entry(end()->prev)->next = get_pointer(&int_entries[bucket]);

    int_entries[bucket].next = get_pointer(end());
    end()->prev = get_pointer(&int_entries[bucket]);

    new(&int_entries[bucket].value) T(v);

    int_size++;

    if (int_key > max_key) {
      max_key = int_key;
    }
  } else {
    if (!save_value) {
      int_entries[bucket].value = v;
    }
  }

  return int_entries[bucket].value;
}

template<class T>
bool array<T>::array_inner::has_key(int int_key) const {
  if (is_vector()) {
    return ((unsigned int)int_key < (unsigned int)int_size);
  }

  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  return int_entries[bucket].next != EMPTY_POINTER;
}

template<class T>
bool array<T>::array_inner::isset_value(int int_key) const {
  return has_key(int_key);
}

template<>
inline bool array<var>::array_inner::isset_value(int int_key) const {
  if (is_vector()) {
    return ((unsigned int)int_key < (unsigned int)int_size && !((var *)int_entries)[int_key].is_null());
  }

  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  return int_entries[bucket].next != EMPTY_POINTER && !int_entries[bucket].value.is_null();
}

template<class T>
void array<T>::array_inner::unset_vector_value() {
  ((T *)int_entries)[max_key].~T();
  max_key--;
  int_size--;
}

template<class T>
void array<T>::array_inner::unset_map_value(int int_key) {
  int bucket = choose_bucket(int_key, int_buf_size);
  while (int_entries[bucket].next != EMPTY_POINTER && int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  if (int_entries[bucket].next != EMPTY_POINTER) {
    int_entries[bucket].int_key = 0;

    get_entry(int_entries[bucket].prev)->next = int_entries[bucket].next;
    get_entry(int_entries[bucket].next)->prev = int_entries[bucket].prev;

    int_entries[bucket].next = EMPTY_POINTER;
    int_entries[bucket].prev = EMPTY_POINTER;

    int_entries[bucket].value.~T();

    int_size--;

#define FIXD(a) ((a) >= int_buf_size ? (a) - int_buf_size : (a))
#define FIXU(a, m) ((a) <= (m) ? (a) + int_buf_size : (a))
    int j, rj, ri = bucket;
    for (j = bucket + 1; 1; j++) {
      rj = FIXD(j);
      if (int_entries[rj].next == EMPTY_POINTER) {
        break;
      }

      int bucket_j = choose_bucket(int_entries[rj].int_key, int_buf_size);
      int wnt = FIXU(bucket_j, bucket);

      if (wnt > j || wnt <= bucket) {
        list_hash_entry *ei = int_entries + ri, *ej = int_entries + rj;
        memcpy(ei, ej, sizeof(int_hash_entry));
        ej->next = EMPTY_POINTER;

        get_entry(ei->prev)->next = get_pointer(ei);
        get_entry(ei->next)->prev = get_pointer(ei);

        ri = rj;
        bucket = j;
      }
    }
#undef FIXU
#undef FIXD
  }
}


template<class T>
const var array<T>::array_inner::get_var(int int_key, const string &string_key) const {
  if (is_vector()) {
    return var();
  }

  const string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  if (string_entries[bucket].next == EMPTY_POINTER) {
    return var();
  }

  return string_entries[bucket].value;
}

template<class T>
const T array<T>::array_inner::get_value(int int_key, const string &string_key) const {
  if (is_vector()) {
    return empty_T;
  }

  const string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  if (string_entries[bucket].next == EMPTY_POINTER) {
    return empty_T;
  }

  return string_entries[bucket].value;
}

template<class T>
T &array<T>::array_inner::set_map_value(int int_key, const string &string_key, const T &v, bool save_value) {
  string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  if (string_entries[bucket].next == EMPTY_POINTER) {
    string_entries[bucket].int_key = int_key;
    new(&string_entries[bucket].string_key) string(string_key);

    string_entries[bucket].prev = end()->prev;
    get_entry(end()->prev)->next = get_pointer(&string_entries[bucket]);

    string_entries[bucket].next = get_pointer(end());
    end()->prev = get_pointer(&string_entries[bucket]);

    new(&string_entries[bucket].value) T(v);

    string_size++;
  } else {
    if (!save_value) {
      string_entries[bucket].value = v;
    }
  }

  return string_entries[bucket].value;
}

template<class T>
bool array<T>::array_inner::has_key(int int_key, const string &string_key) const {
  if (is_vector()) {
    return false;
  }

  const string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  return string_entries[bucket].next != EMPTY_POINTER;
}

template<class T>
bool array<T>::array_inner::isset_value(int int_key, const string &string_key) const {
  return has_key(int_key, string_key);
}

template<>
inline bool array<var>::array_inner::isset_value(int int_key, const string &string_key) const {
  if (is_vector()) {
    return false;
  }

  const string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  return string_entries[bucket].next != EMPTY_POINTER && !string_entries[bucket].value.is_null();
}

template<class T>
void array<T>::array_inner::unset_map_value(int int_key, const string &string_key) {
  string_hash_entry *string_entries = get_string_entries();
  int bucket = choose_bucket(int_key, string_buf_size);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == string_buf_size)) {
      bucket = 0;
    }
  }

  if (string_entries[bucket].next != EMPTY_POINTER) {
    string_entries[bucket].int_key = 0;
    string_entries[bucket].string_key.~string();

    get_entry(string_entries[bucket].prev)->next = string_entries[bucket].next;
    get_entry(string_entries[bucket].next)->prev = string_entries[bucket].prev;

    string_entries[bucket].next = EMPTY_POINTER;
    string_entries[bucket].prev = EMPTY_POINTER;

    string_entries[bucket].value.~T();

    string_size--;

#define FIXD(a) ((a) >= string_buf_size ? (a) - string_buf_size : (a))
#define FIXU(a, m) ((a) <= (m) ? (a) + string_buf_size : (a))
    int j, rj, ri = bucket;
    for (j = bucket + 1; 1; j++) {
      rj = FIXD(j);
      if (string_entries[rj].next == EMPTY_POINTER) {
        break;
      }

      int bucket_j = choose_bucket(string_entries[rj].int_key, string_buf_size);
      int wnt = FIXU(bucket_j, bucket);

      if (wnt > j || wnt <= bucket) {
        list_hash_entry *ei = string_entries + ri, *ej = string_entries + rj;
        memcpy(ei, ej, sizeof(string_hash_entry));
        ej->next = EMPTY_POINTER;

        get_entry(ei->prev)->next = get_pointer(ei);
        get_entry(ei->next)->prev = get_pointer(ei);

        ri = rj;
        bucket = j;
      }
    }
#undef FIXU
#undef FIXD
  }
}


template<class T>
bool array<T>::is_vector() const {
  return p->is_vector();
}

template<class T>
bool array<T>::mutate_if_vector_shared(int mul) {
  return mutate_to_size_if_vector_shared(mul * p->int_size);
}

template<class T>
bool array<T>::mutate_to_size_if_vector_shared(int int_size) {
  if (p->ref_cnt > 0) {
    array_inner *new_array = array_inner::create(int_size, 0, true);

    int size = p->int_size;
    T *it = (T *)p->int_entries;

    for (int i = 0; i < size; i++) {
      new_array->push_back_vector_value(it[i]);
    }

    p->dispose();
    p = new_array;
    return true;
  }
  return false;
}

template<class T>
bool array<T>::mutate_if_map_shared(int mul) {
  if (p->ref_cnt > 0) {
    array_inner *new_array = array_inner::create(p->int_size * mul + 1, p->string_size * mul + 1, false);

    for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        new_array->set_map_value(it->int_key, it->value, false);
      }
    }

    p->dispose();
    p = new_array;
    return true;
  }
  return false;
}

template<class T>
void array<T>::mutate_if_vector_needed_int() {
  if (mutate_if_vector_shared(2)) {
    return;
  }

  if (p->int_size == p->int_buf_size) {
    mutate_to_size(p->int_buf_size * 2);
  }
}

template<class T>
void array<T>::mutate_to_size(int int_size) {
  if (mutate_to_size_if_vector_shared(int_size)) {
    return;
  }
  p = (array_inner *)dl::reallocate((void *)p,
                                    (dl::size_type)(sizeof(array_inner) + int_size * sizeof(T)),
                                    (dl::size_type)(sizeof(array_inner) + p->int_buf_size * sizeof(T)));
  p->int_buf_size = int_size;
}

template<class T>
void array<T>::mutate_if_map_needed_int() {
  if (mutate_if_map_shared(2)) {
    return;
  }

  if (p->int_size * 5 > 3 * p->int_buf_size) {
    int new_int_size = max(p->int_size * 2 + 1, p->string_size);
    int new_string_size = max(p->string_size, (p->string_buf_size >> 1) - 1);
    array_inner *new_array = array_inner::create(new_int_size, new_string_size, false);

    for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        new_array->set_map_value(it->int_key, it->value, false);
      }
    }

    p->dispose();
    p = new_array;
  }
}

template<class T>
void array<T>::mutate_if_map_needed_string() {
  if (mutate_if_map_shared(2)) {
    return;
  }

  if (p->string_size * 5 > 3 * p->string_buf_size) {
    int new_int_size = max(p->int_size, (p->int_buf_size >> 1) - 1);
    int new_string_size = max(p->string_size * 2 + 1, p->int_size);
    array_inner *new_array = array_inner::create(new_int_size, new_string_size, false);

    for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        new_array->set_map_value(it->int_key, it->value, false);
      }
    }

    p->dispose();
    p = new_array;
  }
}

template<class T>
void array<T>::reserve(int int_size, int string_size, bool make_vector_if_possible) {
  if (int_size > p->int_buf_size || (string_size > 0 && string_size > p->string_buf_size)) {
    if (is_vector() && string_size == 0 && make_vector_if_possible) {
      mutate_to_size(int_size);
    } else {
      int new_int_size = max(int_size, p->int_buf_size);
      int new_string_size = max(string_size, p->string_buf_size);
      array_inner *new_array = array_inner::create(new_int_size, new_string_size, false);

      if (is_vector()) {
        for (int it = 0; it != p->int_size; it++) {
          new_array->set_map_value(it, ((T *)p->int_entries)[it], false);
        }
        php_assert (new_array->max_key == p->max_key);
      } else {
        for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
          if (p->is_string_hash_entry(it)) {
            new_array->set_map_value(it->int_key, it->string_key, it->value, false);
          } else {
            new_array->set_map_value(it->int_key, it->value, false);
          }
        }
      }

      p->dispose();
      p = new_array;
    }
  }
}

template<class T>
template<class Arg, class... Args>
void array<T>::push_back_values(Arg &&arg, Args &&... args) {
  static_assert(std::is_convertible<typename std::decay<Arg>::type, T>::value, "Arg type must be convertible to T");

  p->push_back_vector_value(std::forward<Arg>(arg));
  push_back_values(std::forward<Args>(args)...);
}

template<class T>
array<T>::const_iterator::const_iterator():
  self(NULL),
  entry(NULL) {
}

template<class T>
array<T>::const_iterator::const_iterator(const typename array<T>::array_inner *self, const list_hash_entry *entry):
  self(self),
  entry(entry) {
}

template<class T>
const T &array<T>::const_iterator::get_value() const {
  if (self->is_vector()) {
    return *reinterpret_cast<const T *>(entry);
  }

  return reinterpret_cast<const int_hash_entry *>(entry)->value;
}

template<class T>
typename array<T>::key_type array<T>::const_iterator::get_key() const {
  if (self->is_vector()) {
    return key_type((int)((const T *)entry - (const T *)self->int_entries));
  }

  if (self->is_string_hash_entry((const string_hash_entry *)entry)) {
    return ((const string_hash_entry *)entry)->get_key();
  } else {
    return ((const int_hash_entry *)entry)->get_key();
  }
}

template<class T>
typename array<T>::const_iterator &array<T>::const_iterator::operator--() {
  if (self->is_vector()) {
    entry = (const list_hash_entry *)((const T *)entry - 1);
  } else {
    entry = (const list_hash_entry *)self->prev((const string_hash_entry *)entry);
  }
  return *this;
}

template<class T>
typename array<T>::const_iterator &array<T>::const_iterator::operator++() {
  if (self->is_vector()) {
    entry = (const list_hash_entry *)((const T *)entry + 1);
  } else {
    entry = (const list_hash_entry *)self->next((const string_hash_entry *)entry);
  }
  return *this;
}

template<class T>
bool array<T>::const_iterator::operator==(const array<T>::const_iterator &other) const {
  return entry == other.entry;
}

template<class T>
bool array<T>::const_iterator::operator!=(const array<T>::const_iterator &other) const {
  return entry != other.entry;
}


template<class T>
typename array<T>::const_iterator array<T>::begin() const {
  if (is_vector()) {
    return typename array<T>::const_iterator(p, p->int_entries);
  }

  return typename array<T>::const_iterator(p, (const list_hash_entry *)p->begin());
}

template<class T>
typename array<T>::const_iterator array<T>::middle(int n) const {
  int l = count();

  if (is_vector()) {
    if (n < 0) {
      n += l;
      if (n < 0) {
        return end();
      }
    }
    if (n >= l) {
      return end();
    }

    return typename array<T>::const_iterator(p, (list_hash_entry * )((T *)p->int_entries + n));
  }

  if (n < -l / 2) {
    n += l;
    if (n < 0) {
      return end();
    }
  }

  if (n > l / 2) {
    n -= l;
    if (n >= 0) {
      return end();
    }
  }

  const string_hash_entry *result;
  if (n < 0) {
    result = p->end();
    while (n < 0) {
      n++;
      result = p->prev(result);
    }
  } else {
    result = p->begin();
    while (n > 0) {
      n--;
      result = p->next(result);
    }
  }
  return typename array<T>::const_iterator(p, (const list_hash_entry *)result);
}

template<class T>
typename array<T>::const_iterator array<T>::end() const {
  if (is_vector()) {
    return typename array<T>::const_iterator(p, (const list_hash_entry *)((const T *)p->int_entries + p->int_size));
  }

  return typename array<T>::const_iterator(p, (const list_hash_entry *)p->end());
}


template<class T>
array<T>::iterator::iterator():
  self(NULL),
  entry(NULL) {
}

template<class T>
array<T>::iterator::iterator(typename array<T>::array_inner *self, list_hash_entry *entry):
  self(self),
  entry(entry) {
}

template<class T>
T &array<T>::iterator::get_value() {
  if (self->is_vector()) {
    return *reinterpret_cast<T *>(entry);
  }

  return reinterpret_cast<int_hash_entry *>(entry)->value;
}

template<class T>
typename array<T>::key_type array<T>::iterator::get_key() const {
  if (self->is_vector()) {
    return key_type((int)((const T *)entry - (const T *)self->int_entries));
  }

  if (self->is_string_hash_entry((const string_hash_entry *)entry)) {
    return ((const string_hash_entry *)entry)->get_key();
  } else {
    return ((const int_hash_entry *)entry)->get_key();
  }
}

template<class T>
typename array<T>::iterator &array<T>::iterator::operator--() {
  if (self->is_vector()) {
    entry = (list_hash_entry * )((T *)entry - 1);
  } else {
    entry = (list_hash_entry *)self->prev((string_hash_entry *)entry);
  }
  return *this;
}

template<class T>
typename array<T>::iterator &array<T>::iterator::operator++() {
  if (self->is_vector()) {
    entry = (list_hash_entry * )((T *)entry + 1);
  } else {
    entry = (list_hash_entry *)self->next((string_hash_entry *)entry);
  }
  return *this;
}

template<class T>
bool array<T>::iterator::operator==(const array<T>::iterator &other) const {
  return entry == other.entry;
}

template<class T>
bool array<T>::iterator::operator!=(const array<T>::iterator &other) const {
  return entry != other.entry;
}


template<class T>
typename array<T>::iterator array<T>::begin() {
  if (is_vector()) {
    mutate_if_vector_shared();
    return typename array<T>::iterator(p, p->int_entries);
  }

  mutate_if_map_shared();
  return typename array<T>::iterator(p, p->begin());
}

template<class T>
typename array<T>::iterator array<T>::middle(int n) {
  int l = count();

  if (is_vector()) {
    if (n < 0) {
      n += l;
      if (n < 0) {
        return end();
      }
    }
    if (n >= l) {
      return end();
    }

    return typename array<T>::iterator(p, (list_hash_entry * )((T *)p->int_entries + n));
  }

  if (n < -l / 2) {
    n += l;
    if (n < 0) {
      return end();
    }
  }

  if (n > l / 2) {
    n -= l;
    if (n >= 0) {
      return end();
    }
  }

  string_hash_entry *result;
  if (n < 0) {
    result = p->end();
    while (n < 0) {
      n++;
      result = p->prev(result);
    }
  } else {
    result = p->begin();
    while (n > 0) {
      n--;
      result = p->next(result);
    }
  }
  return typename array<T>::iterator(p, result);
}

template<class T>
typename array<T>::iterator array<T>::end() {
  if (is_vector()) {
    return typename array<T>::iterator(p, (list_hash_entry * )((T *)p->int_entries + p->int_size));
  }

  return typename array<T>::iterator(p, p->end());
}


template<class T>
void array<T>::convert_to_map() {
  array_inner *new_array = array_inner::create(p->int_size + 4, p->int_size + 4, false);

  for (int it = 0; it != p->int_size; it++) {
    new_array->set_map_value(it, ((T *)p->int_entries)[it], false);
  }

  php_assert (new_array->max_key == p->max_key);

  p->dispose();
  p = new_array;
}

template<class T>
template<class T1>
void array<T>::copy_from(const array<T1> &other) {
  if (other.empty()) {
    p = array_inner::empty_array();
    return;
  }

  array_inner *new_array = array_inner::create(other.p->int_size, other.p->string_size, other.is_vector());

  if (new_array->is_vector()) {
    int size = other.p->int_size;
    T1 *it = (T1 *)other.p->int_entries;

    for (int i = 0; i < size; i++) {
      new_array->push_back_vector_value(convert_to<T>::convert(it[i]));
    }
  } else {
    for (const typename array<T1>::string_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, convert_to<T>::convert(it->value), false);
      } else {
        new_array->set_map_value(it->int_key, convert_to<T>::convert(it->value), false);
      }
    }
  }

  p = new_array;

  php_assert (new_array->int_size == other.p->int_size);
  php_assert (new_array->string_size == other.p->string_size);
}

template<class T>
template<class U>
array<T> array<T>::convert_from(const array<U> &other) {
  array<T> res;
  res.copy_from(other);
  return res;
}

template<class T>
array<T>::array():
  p(array_inner::empty_array()) {
}


template<class T>
array<T>::array(const array_size &s) :
  p(array_inner::create(s.int_size, s.string_size, s.is_vector)) {
}

template<class T>
template<class KeyT>
inline array<T>::array(const std::initializer_list<std::pair<KeyT, T>> &list) :
  array() {
  for (const auto &kv: list) {
    set_value(kv.first, kv.second);
  }
}


template<class T>
array<T>::array(const array<T> &other) :
  p(other.p->ref_copy()) {}

template<class T>
array<T>::array(array<T> &&other) noexcept :
  p(other.p) {
  other.p = nullptr;
}

template<class T>
template<class T1, class>
array<T>::array(const array<T1> &other) {
  copy_from(other);
}

template<class T>
template<class... Args>
inline array<T> array<T>::create(Args &&... args) {
  array<T> res{array_size{sizeof...(args), 0, true}};
  res.push_back_values(std::forward<Args>(args)...);
  return res;
}

template<class T>
array<T> &array<T>::operator=(const array &other) {
  typename array::array_inner *other_copy = other.p->ref_copy();
  destroy();
  p = other_copy;
  return *this;
}

template<class T>
array<T> &array<T>::operator=(array &&other) noexcept {
  typename array::array_inner *other_copy = other.p;
  other.p = nullptr;
  destroy();
  p = other_copy;
  return *this;
}

template<class T>
template<class T1, class>
array<T> &array<T>::operator=(const array<T1> &other) {
  typename array<T1>::array_inner *other_copy = other.p->ref_copy();
  destroy();
  copy_from(other);
  other_copy->dispose();
  return *this;
}

template<class T>
void array<T>::destroy() {
  if (p) {
    p->dispose();
  }
}

template<class T>
array<T>::~array() {
  destroy();
}


template<class T>
void array<T>::clear() {
  destroy();
  p = array_inner::empty_array();
}


template<class T>
T &array<T>::operator[](int int_key) {
  if (is_vector()) {
    if ((unsigned int)int_key <= (unsigned int)p->int_size) {
      if ((unsigned int)int_key == (unsigned int)p->int_size) {
        mutate_if_vector_needed_int();
        return p->push_back_vector_value(array_inner::empty_T);
      } else {
        mutate_if_vector_shared();
        return p->get_vector_value(int_key);
      }
    }

    convert_to_map();
  } else {
    mutate_if_map_needed_int();
  }

  return p->set_map_value(int_key, array_inner::empty_T, true);
}

template<class T>
T &array<T>::operator[](const string &string_key) {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return (*this)[int_val];
  }

  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_needed_string();
  }

  return p->set_map_value(string_key.hash(), string_key, array_inner::empty_T, true);
}

template<class T>
T &array<T>::operator[](const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return (*this)[string()];
    case var::BOOLEAN_TYPE:
      return (*this)[v.as_bool()];
    case var::INTEGER_TYPE:
      return (*this)[v.as_int()];
    case var::FLOAT_TYPE:
      return (*this)[(int)v.as_double()];
    case var::STRING_TYPE:
      return (*this)[v.as_string()];
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return (*this)[v.as_array().to_int()];
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
T &array<T>::operator[](const const_iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key <= (unsigned int)p->int_size) {
        if ((unsigned int)key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          return p->push_back_vector_value(array_inner::empty_T);
        } else {
          mutate_if_vector_shared();
          return p->get_vector_value(key);
        }
      }

      convert_to_map();
    } else {
      mutate_if_map_needed_int();
    }

    return p->set_map_value(key, array_inner::empty_T, true);
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (!is_string_entry && (unsigned int)entry->int_key <= (unsigned int)p->int_size) {
        if ((unsigned int)entry->int_key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          return p->push_back_vector_value(array_inner::empty_T);
        } else {
          mutate_if_vector_shared();
          return p->get_vector_value(entry->int_key);
        }
      }

      convert_to_map();
    } else {
      if (is_string_entry) {
        mutate_if_map_needed_string();
      } else {
        mutate_if_map_needed_int();
      }
    }

    if (is_string_entry) {
      return p->set_map_value(entry->int_key, entry->string_key, array_inner::empty_T, true);
    } else {
      return p->set_map_value(entry->int_key, array_inner::empty_T, true);
    }
  }
}

template<class T>
T &array<T>::operator[](const iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key <= (unsigned int)p->int_size) {
        if ((unsigned int)key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          return p->push_back_vector_value(array_inner::empty_T);
        } else {
          mutate_if_vector_shared();
          return p->get_vector_value(key);
        }
      }

      convert_to_map();
    } else {
      mutate_if_map_needed_int();
    }

    return p->set_map_value(key, array_inner::empty_T, true);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (!is_string_entry && (unsigned int)entry->int_key <= (unsigned int)p->int_size) {
        if ((unsigned int)entry->int_key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          return p->push_back_vector_value(array_inner::empty_T);
        } else {
          mutate_if_vector_shared();
          return p->get_vector_value(entry->int_key);
        }
      }

      convert_to_map();
    } else {
      if (is_string_entry) {
        mutate_if_map_needed_string();
      } else {
        mutate_if_map_needed_int();
      }
    }

    if (is_string_entry) {
      return p->set_map_value(entry->int_key, entry->string_key, array_inner::empty_T, true);
    } else {
      return p->set_map_value(entry->int_key, array_inner::empty_T, true);
    }
  }
}


template<class T>
void array<T>::set_value(int int_key, const T &v) {
  if (is_vector()) {
    if ((unsigned int)int_key <= (unsigned int)p->int_size) {
      if ((unsigned int)int_key == (unsigned int)p->int_size) {
        mutate_if_vector_needed_int();
        p->push_back_vector_value(v);
      } else {
        mutate_if_vector_shared();
        p->set_vector_value(int_key, v);
      }
      return;
    }

    convert_to_map();
  } else {
    mutate_if_map_needed_int();
  }

  p->set_map_value(int_key, v, false);
}

template<class T>
void array<T>::set_value(const string &string_key, const T &v) {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    set_value(int_val, v);
    return;
  }

  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_needed_string();
  }

  p->set_map_value(string_key.hash(), string_key, v, false);
}

template<class T>
void array<T>::set_value(const string &string_key, const T &v, int precomuted_hash) {
  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_needed_string();
  }

  p->set_map_value(precomuted_hash, string_key, v, false);
}

template<class T>
void array<T>::set_value(const var &v, const T &value) {
  switch (v.type) {
    case var::NULL_TYPE:
      return set_value(string(), value);
    case var::BOOLEAN_TYPE:
      return set_value(v.as_bool(), value);
    case var::INTEGER_TYPE:
      return set_value(v.as_int(), value);
    case var::FLOAT_TYPE:
      return set_value((int)v.as_double(), value);
    case var::STRING_TYPE:
      return set_value(v.as_string(), value);
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return set_value(v.as_array().to_int(), value);
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
template<class OrFalseT>
void array<T>::set_value(const OrFalse<OrFalseT> &key, const T &value) {
  if (!key.bool_value) {
    return set_value(false, value);
  }

  set_value(key.val(), value);
}

template<class T>
void array<T>::set_value(const const_iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key <= (unsigned int)p->int_size) {
        if ((unsigned int)key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          p->push_back_vector_value(*(T *)it.entry);
        } else {
          mutate_if_vector_shared();
          p->set_vector_value(key, *(T *)it.entry);
        }
        return;
      }

      convert_to_map();
    } else {
      mutate_if_map_needed_int();
    }

    p->set_map_value(key, *(T *)it.entry, false);
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (!is_string_entry && (unsigned int)entry->int_key <= (unsigned int)p->int_size) {
        if ((unsigned int)entry->int_key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          p->push_back_vector_value(entry->value);
        } else {
          mutate_if_vector_shared();
          p->set_vector_value(entry->int_key, entry->value);
        }
        return;
      }

      convert_to_map();
    } else {
      if (is_string_entry) {
        mutate_if_map_needed_string();
      } else {
        mutate_if_map_needed_int();
      }
    }

    if (is_string_entry) {
      p->set_map_value(entry->int_key, entry->string_key, entry->value, false);
    } else {
      p->set_map_value(entry->int_key, entry->value, false);
    }
  }
}

template<class T>
void array<T>::set_value(const iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key <= (unsigned int)p->int_size) {
        if ((unsigned int)key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          p->push_back_vector_value(*(T *)it.entry);
        } else {
          mutate_if_vector_shared();
          p->set_vector_value(key, *(T *)it.entry);
        }
        return;
      }

      convert_to_map();
    } else {
      mutate_if_map_needed_int();
    }

    p->set_map_value(key, *(T *)it.entry, false);
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (!is_string_entry && (unsigned int)entry->int_key <= (unsigned int)p->int_size) {
        if ((unsigned int)entry->int_key == (unsigned int)p->int_size) {
          mutate_if_vector_needed_int();
          p->push_back_vector_value(entry->value);
        } else {
          mutate_if_vector_shared();
          p->set_vector_value(entry->int_key, entry->value);
        }
        return;
      }

      convert_to_map();
    } else {
      if (is_string_entry) {
        mutate_if_map_needed_string();
      } else {
        mutate_if_map_needed_int();
      }
    }

    if (is_string_entry) {
      p->set_map_value(entry->int_key, entry->string_key, entry->value, false);
    } else {
      p->set_map_value(entry->int_key, entry->value, false);
    }
  }
}


template<class T>
void array<T>::assign_raw(const char *s) {
  php_assert(sizeof(array_inner) == 8 * sizeof(int));
  p = reinterpret_cast<array_inner *>(const_cast<char *>(s));
}

template<class T>
const var array<T>::get_var(int int_key) const {
  return p->get_var(int_key);
}

template<class T>
const var array<T>::get_var(const string &string_key) const {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return p->get_var(int_val);
  }

  return p->get_var(string_key.hash(), string_key);
}

template<class T>
const var array<T>::get_var(const var &v) const {
  switch (v.type) {
    case var::NULL_TYPE:
      return get_var(string());
    case var::BOOLEAN_TYPE:
      return get_var(v.as_bool());
    case var::INTEGER_TYPE:
      return get_var(v.as_int());
    case var::FLOAT_TYPE:
      return get_var((int)v.as_double());
    case var::STRING_TYPE:
      return get_var(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return get_var(v.as_array().to_int());
    default:
      php_assert (0);
      exit(1);
  }
}


template<class T>
const T array<T>::get_value(int int_key) const {
  return p->get_value(int_key);
}

template<class T>
const T array<T>::get_value(const string &string_key) const {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return p->get_value(int_val);
  }

  return p->get_value(string_key.hash(), string_key);
}

template<class T>
const T array<T>::get_value(const string &string_key, int precomuted_hash) const {
  return p->get_value(precomuted_hash, string_key);
}

template<class T>
const T array<T>::get_value(const var &v) const {
  switch (v.type) {
    case var::NULL_TYPE:
      return get_value(string());
    case var::BOOLEAN_TYPE:
      return get_value(v.as_bool());
    case var::INTEGER_TYPE:
      return get_value(v.as_int());
    case var::FLOAT_TYPE:
      return get_value((int)v.as_double());
    case var::STRING_TYPE:
      return get_value(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return get_value(v.as_array().to_int());
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
const T array<T>::get_value(const const_iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->get_value(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->get_value(entry->int_key, entry->string_key);
    } else {
      return p->get_value(entry->int_key);
    }
  }
}

template<class T>
const T array<T>::get_value(const iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->get_value(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->get_value(entry->int_key, entry->string_key);
    } else {
      return p->get_value(entry->int_key);
    }
  }
}


template<class T>
bool array<T>::has_key(int int_key) const {
  return p->has_key(int_key);
}

template<class T>
bool array<T>::has_key(const string &string_key) const {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return p->has_key(int_val);
  }

  return p->has_key(string_key.hash(), string_key);
}

template<class T>
bool array<T>::has_key(const var &v) const {
  switch (v.type) {
    case var::NULL_TYPE:
      return has_key(string());
    case var::BOOLEAN_TYPE:
      return has_key(v.as_bool());
    case var::INTEGER_TYPE:
      return has_key(v.as_int());
    case var::FLOAT_TYPE:
      return has_key((int)v.as_double());
    case var::STRING_TYPE:
      return has_key(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return has_key(v.as_array().to_int());
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
bool array<T>::has_key(const const_iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->has_key(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->has_key(entry->int_key, entry->string_key);
    } else {
      return p->has_key(entry->int_key);
    }
  }
}

template<class T>
bool array<T>::has_key(const iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->has_key(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->has_key(entry->int_key, entry->string_key);
    } else {
      return p->has_key(entry->int_key);
    }
  }
}

template<class T>
bool array<T>::isset(int int_key) const {
  return p->isset_value(int_key);
}

template<class T>
bool array<T>::isset(const string &string_key) const {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return p->isset_value(int_val);
  }

  return p->isset_value(string_key.hash(), string_key);
}

template<class T>
bool array<T>::isset(const var &v) const {
  switch (v.type) {
    case var::NULL_TYPE:
      return isset(string());
    case var::BOOLEAN_TYPE:
      return isset(v.as_bool());
    case var::INTEGER_TYPE:
      return isset(v.as_int());
    case var::FLOAT_TYPE:
      return isset((int)v.as_double());
    case var::STRING_TYPE:
      return isset(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return isset(v.as_array().to_int());
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
bool array<T>::isset(const const_iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->isset_value(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->isset_value(entry->int_key, entry->string_key);
    } else {
      return p->isset_value(entry->int_key);
    }
  }
}

template<class T>
bool array<T>::isset(const iterator &it) const {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    return p->isset_value(key);
  } else {
    const string_hash_entry *entry = (const string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      return p->isset_value(entry->int_key, entry->string_key);
    } else {
      return p->isset_value(entry->int_key);
    }
  }
}


template<class T>
void array<T>::unset(int int_key) {
  if (is_vector()) {
    if ((unsigned int)int_key >= (unsigned int)p->int_size) {
      return;
    }
    if (int_key == p->max_key) {
      mutate_if_vector_shared();
      return p->unset_vector_value();
    }
    convert_to_map();
  } else {
    mutate_if_map_shared();
  }

  return p->unset_map_value(int_key);
}

template<class T>
void array<T>::unset(const string &string_key) {
  int int_val;
  if ((unsigned int)(string_key[0] - '-') < 13u && string_key.try_to_int(&int_val)) {
    return unset(int_val);
  }

  if (is_vector()) {
    return;
  }

  mutate_if_map_shared();
  return p->unset_map_value(string_key.hash(), string_key);
}

template<class T>
void array<T>::unset(const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return unset(string());
    case var::BOOLEAN_TYPE:
      return unset(v.as_bool());
    case var::INTEGER_TYPE:
      return unset(v.as_int());
    case var::FLOAT_TYPE:
      return unset((int)v.as_double());
    case var::STRING_TYPE:
      return unset(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type array");
      return unset(v.as_array().to_int());
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
void array<T>::unset(const const_iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key >= (unsigned int)p->int_size) {
        return;
      }

      if (key == p->max_key) {
        mutate_if_vector_shared();
        return p->unset_vector_value();
      }

      convert_to_map();
    } else {
      mutate_if_map_shared();
    }

    return p->unset_map_value(key);
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (is_string_entry || (unsigned int)entry->int_key >= (unsigned int)p->int_size) {
        return;
      }

      if (entry->int_key == p->max_key) {
        mutate_if_vector_shared();
        return p->unset_vector_value();
      }

      convert_to_map();
    } else {
      mutate_if_map_shared();
    }

    if (is_string_entry) {
      return p->unset_map_value(entry->int_key, entry->string_key);
    } else {
      return p->unset_map_value(entry->int_key);
    }
  }
}

template<class T>
void array<T>::unset(const iterator &it) {
  if (it.self->is_vector()) {
    int key = (int)((T *)it.entry - (T *)it.self->int_entries);

    if (is_vector()) {
      if ((unsigned int)key >= (unsigned int)p->int_size) {
        return;
      }

      if (key == p->max_key) {
        mutate_if_vector_shared();
        return p->unset_vector_value();
      }

      convert_to_map();
    } else {
      mutate_if_map_shared();
    }

    return p->unset_map_value(key);
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;
    bool is_string_entry = it.self->is_string_hash_entry(entry);

    if (is_vector()) {
      if (is_string_entry || (unsigned int)entry->int_key >= (unsigned int)p->int_size) {
        return;
      }

      if (entry->int_key == p->max_key) {
        mutate_if_vector_shared();
        return p->unset_vector_value();
      }

      convert_to_map();
    } else {
      mutate_if_map_shared();
    }

    if (is_string_entry) {
      return p->unset_map_value(entry->int_key, entry->string_key);
    } else {
      return p->unset_map_value(entry->int_key);
    }
  }
}


template<class T>
bool array<T>::empty() const {
  return p->int_size + p->string_size == 0;
}

template<class T>
int array<T>::count() const {
  return p->int_size + p->string_size;
}

template<class T>
array_size array<T>::size() const {
  return array_size(p->int_size, p->string_size, is_vector());
}

template<class T>
template<class T1, class>
void array<T>::merge_with(const array<T1> &other) {
  for (typename array<T1>::const_iterator it = other.begin(); it != other.end(); ++it) {
    if (it.self->is_vector()) {//TODO move if outside for
      if (is_vector()) {
        mutate_if_vector_needed_int();
        p->push_back_vector_value(*reinterpret_cast<const T1 *>(it.entry));
      } else {
        mutate_if_map_needed_int();
        p->set_map_value(get_next_key(), *reinterpret_cast<const T1 *>(it.entry), false);
      }
    } else {
      const typename array<T1>::string_hash_entry *entry = (const typename array<T1>::string_hash_entry *)it.entry;
      const T1 &value = entry->value;

      if (it.self->is_string_hash_entry(entry)) {
        if (is_vector()) {
          convert_to_map();
        } else {
          mutate_if_map_needed_string();
        }

        p->set_map_value(entry->int_key, entry->string_key, value, false);
      } else {
        if (is_vector()) {
          mutate_if_vector_needed_int();
          p->push_back_vector_value(value);
        } else {
          mutate_if_map_needed_int();
          p->set_map_value(get_next_key(), value, false);
        }
      }
    }
  }
}

template<class T>
const array<T> array<T>::operator+(const array<T> &other) const {
  array<T> result(size() + other.size());

  if (is_vector()) {
    int size = p->int_size;
    T *it = (T *)p->int_entries;

    if (result.is_vector()) {
      for (int i = 0; i < size; i++) {
        result.p->push_back_vector_value(it[i]);
      }
    } else {
      for (int i = 0; i < size; i++) {
        result.p->set_map_value(i, it[i], false);
      }
    }
  } else {
    for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        result.p->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        result.p->set_map_value(it->int_key, it->value, false);
      }
    }
  }

  if (other.is_vector()) {
    int size = other.p->int_size;
    T *it = (T *)other.p->int_entries;

    if (result.is_vector()) {
      for (int i = p->int_size; i < size; i++) {
        result.p->push_back_vector_value(it[i]);
      }
    } else {
      for (int i = 0; i < size; i++) {
        result.p->set_map_value(i, it[i], true);
      }
    }
  } else {
    for (const string_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        result.p->set_map_value(it->int_key, it->string_key, it->value, true);
      } else {
        result.p->set_map_value(it->int_key, it->value, true);
      }
    }
  }

  return result;
}

template<class T>
array<T> &array<T>::operator+=(const array<T> &other) {
  if (is_vector()) {
    if (other.is_vector()) {
      int size = other.p->int_size;
      T *it = (T *)other.p->int_entries;

      if (p->ref_cnt > 0) {
        int my_size = p->int_size;
        T *my_it = (T *)p->int_entries;

        array_inner *new_array = array_inner::create(max(size, my_size), 0, true);

        for (int i = 0; i < my_size; i++) {
          new_array->push_back_vector_value(my_it[i]);
        }

        p->dispose();
        p = new_array;
      } else if (p->int_buf_size < size + 2) {
        int new_size = max(size + 2, p->int_buf_size * 2);
        p = (array_inner *)dl::reallocate((void *)p,
                                          (dl::size_type)(sizeof(array_inner) + new_size * sizeof(T)),
                                          (dl::size_type)(sizeof(array_inner) + p->int_buf_size * sizeof(T)));
        p->int_buf_size = new_size;
      }

      if (p->int_size > 0 && size > 0) {
        php_warning("Strange usage of array operator += on two vectors. Did you mean array_merge?");
      }

      for (int i = p->int_size; i < size; i++) {
        p->push_back_vector_value(it[i]);
      }

      return *this;
    } else {
      array_inner *new_array = array_inner::create(p->int_size + other.p->int_size + 4, other.p->string_size + 4, false);
      T *it = (T *)p->int_entries;

      for (int i = 0; i != p->int_size; i++) {
        new_array->set_map_value(i, it[i], false);
      }

      p->dispose();
      p = new_array;
    }
  } else {
    if (p == other.p) {
      return *this;
    }

    int new_int_size = p->int_size + other.p->int_size;
    int new_string_size = p->string_size + other.p->string_size;

    if (new_int_size * 5 > 3 * p->int_buf_size || new_string_size * 5 > 3 * p->string_buf_size || p->ref_cnt > 0) {
      array_inner *new_array = array_inner::create(max(new_int_size, 2 * p->int_size) + 1, max(new_string_size, 2 * p->string_size) + 1, false);

      for (const string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
        if (p->is_string_hash_entry(it)) {
          new_array->set_map_value(it->int_key, it->string_key, it->value, false);
        } else {
          new_array->set_map_value(it->int_key, it->value, false);
        }
      }

      p->dispose();
      p = new_array;
    }
  }

  if (other.is_vector()) {
    int size = other.p->int_size;
    T *it = (T *)other.p->int_entries;

    for (int i = 0; i < size; i++) {
      p->set_map_value(i, it[i], true);
    }
  } else {
    for (string_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        p->set_map_value(it->int_key, it->string_key, it->value, true);
      } else {
        p->set_map_value(it->int_key, it->value, true);
      }
    }
  }

  return *this;
}


template<class T>
void array<T>::push_back(const T &v) {
  if (is_vector()) {
    mutate_if_vector_needed_int();
    p->push_back_vector_value(v);
  } else {
    mutate_if_map_needed_int();
    p->set_map_value(get_next_key(), v, false);
  }
}

template<class T>
void array<T>::push_back(const const_iterator &it) {
  if (it.self->is_vector()) {
    if (is_vector()) {
      mutate_if_vector_needed_int();
      p->push_back_vector_value(*(T *)it.entry);
    } else {
      mutate_if_map_needed_int();
      p->set_map_value(get_next_key(), *(T *)it.entry, false);
    }
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      if (is_vector()) {
        convert_to_map();
      } else {
        mutate_if_map_needed_string();
      }

      p->set_map_value(entry->int_key, entry->string_key, entry->value, false);
    } else {
      if (is_vector()) {
        mutate_if_vector_needed_int();
        p->push_back_vector_value(entry->value);
      } else {
        mutate_if_map_needed_int();
        p->set_map_value(get_next_key(), entry->value, false);
      }
    }
  }
}

template<class T>
void array<T>::push_back(const iterator &it) {
  if (it.self->is_vector()) {
    if (is_vector()) {
      mutate_if_vector_needed_int();
      p->push_back_vector_value(*(T *)it.entry);
    } else {
      mutate_if_map_needed_int();
      p->set_map_value(get_next_key(), *(T *)it.entry, false);
    }
  } else {
    string_hash_entry *entry = (string_hash_entry *)it.entry;

    if (it.self->is_string_hash_entry(entry)) {
      if (is_vector()) {
        convert_to_map();
      } else {
        mutate_if_map_needed_string();
      }

      p->set_map_value(entry->int_key, entry->string_key, entry->value, false);
    } else {
      if (is_vector()) {
        mutate_if_vector_needed_int();
        p->push_back_vector_value(entry->value);
      } else {
        mutate_if_map_needed_int();
        p->set_map_value(get_next_key(), entry->value, false);
      }
    }
  }
}

template<class T>
const T array<T>::push_back_return(const T &v) {
  if (is_vector()) {
    mutate_if_vector_needed_int();
    return p->push_back_vector_value(v);
  } else {
    mutate_if_map_needed_int();
    return p->set_map_value(get_next_key(), v, false);
  }
}

template<class T>
void array<T>::fill_vector(int num, const T &value) {
  php_assert(is_vector() && p->int_size == 0 && num <= p->int_buf_size);

  std::uninitialized_fill((T *)p->int_entries, (T *)p->int_entries + num, value);
  p->max_key = num - 1;
  p->int_size = num;
}


template<class T>
int array<T>::get_next_key() const {
  return p->max_key + 1;
}


template<class T>
template<class T1>
void array<T>::sort(const T1 &compare, bool renumber) {
  int n = count();

  if (renumber) {
    if (n == 0) {
      return;
    }

    if (!is_vector()) {
      array_inner *res = array_inner::create(n, 0, true);
      for (string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
        res->push_back_vector_value(it->value);
      }

      p->dispose();
      p = res;
    } else {
      mutate_if_vector_shared();
    }

    const auto elements_cmp =
      [&compare](const T &lhs, const T &rhs) {
        return compare(lhs, rhs) > 0;
      };
    T *begin = reinterpret_cast<T *>(p->int_entries);
    dl::sort<T, decltype(elements_cmp)>(begin, begin + n, elements_cmp);
    return;
  }

  if (n <= 1) {
    return;
  }

  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_shared();
  }

  int_hash_entry **arTmp = (int_hash_entry **)dl::allocate((dl::size_type)(n * sizeof(int_hash_entry * )));
  int i = 0;
  for (string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
    arTmp[i++] = (int_hash_entry *)it;
  }
  php_assert (i == n);

  const auto hash_entry_cmp =
    [&compare](const int_hash_entry *lhs, const int_hash_entry *rhs) {
      return compare(lhs->value, rhs->value) > 0;
    };
  dl::sort<int_hash_entry *, decltype(hash_entry_cmp)>(arTmp, arTmp + n, hash_entry_cmp);

  arTmp[0]->prev = p->get_pointer(p->end());
  p->end()->next = p->get_pointer(arTmp[0]);
  for (int j = 1; j < n; j++) {
    arTmp[j]->prev = p->get_pointer(arTmp[j - 1]);
    arTmp[j - 1]->next = p->get_pointer(arTmp[j]);
  }
  arTmp[n - 1]->next = p->get_pointer(p->end());
  p->end()->prev = p->get_pointer(arTmp[n - 1]);

  dl::deallocate(arTmp, (dl::size_type)(n * sizeof(int_hash_entry * )));
}


template<class T>
template<class T1>
void array<T>::ksort(const T1 &compare) {
  int n = count();
  if (n <= 1) {
    return;
  }

  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_shared();
  }

  array<key_type> keys(array_size(n, 0, true));
  for (string_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
    if (p->is_string_hash_entry(it)) {
      keys.p->push_back_vector_value(it->get_key());
    } else {
      keys.p->push_back_vector_value(((int_hash_entry *)it)->get_key());
    }
  }

  key_type *keysp = (key_type *)keys.p->int_entries;
  dl::sort<key_type, T1>(keysp, keysp + n, compare);

  list_hash_entry *prev = (list_hash_entry *)p->end();
  for (int j = 0; j < n; j++) {
    list_hash_entry *cur;
    if (is_int_key(keysp[j])) {
      int int_key = keysp[j].to_int();
      int bucket = array_inner::choose_bucket(int_key, p->int_buf_size);
      while (p->int_entries[bucket].int_key != int_key) {
        if (unlikely (++bucket == p->int_buf_size)) {
          bucket = 0;
        }
      }
      cur = (list_hash_entry * ) & p->int_entries[bucket];
    } else {
      string string_key = keysp[j].to_string();
      int int_key = string_key.hash();
      string_hash_entry *string_entries = p->get_string_entries();
      int bucket = array_inner::choose_bucket(int_key, p->string_buf_size);
      while ((string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
        if (unlikely (++bucket == p->string_buf_size)) {
          bucket = 0;
        }
      }
      cur = (list_hash_entry * ) & string_entries[bucket];
    }

    cur->prev = p->get_pointer(prev);
    prev->next = p->get_pointer(cur);

    prev = cur;
  }
  prev->next = p->get_pointer(p->end());
  p->end()->prev = p->get_pointer(prev);
}


template<class T>
void array<T>::swap(array<T> &other) {
  array_inner *tmp = p;
  p = other.p;
  other.p = tmp;
}

template<class T>
T array<T>::pop() {
  if (empty()) {
//    php_warning ("Cannot use function array_pop on empty array");
    return array_inner::empty_T;
  }

  if (is_vector()) {
    mutate_if_vector_shared();

    T *it = (T *)p->int_entries;
    T result = it[p->max_key];

    p->unset_vector_value();

    return result;
  } else {
    mutate_if_map_shared();

    string_hash_entry *it = p->prev(p->end());
    T result = it->value;

    if (p->is_string_hash_entry(it)) {
      p->unset_map_value(it->int_key, it->string_key);
    } else {
      p->unset_map_value(it->int_key);
    }

    return result;
  }
}

template<class T>
T array<T>::shift() {
  if (count() == 0) {
    php_warning("Cannot use array_shift on empty array");
    return array_inner::empty_T;
  }

  if (is_vector()) {
    mutate_if_vector_shared();

    T *it = (T *)p->int_entries;
    T res = *it;

    it->~T();
    memmove((void *)it, it + 1, --p->int_size * sizeof(T));
    p->max_key--;

    return res;
  } else {
    array_size new_size = size().cut(count() - 1);
    bool is_v = (new_size.string_size == 0);

    array_inner *new_array = array_inner::create(new_size.int_size, new_size.string_size, is_v);
    string_hash_entry *it = p->begin();
    T res = it->value;

    it = p->next(it);
    while (it != p->end()) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        if (is_v) {
          new_array->push_back_vector_value(it->value);
        } else {
          new_array->set_map_value(new_array->max_key + 1, it->value, false);
        }
      }

      it = p->next(it);
    }

    p->dispose();
    p = new_array;

    return res;
  }
}

template<class T>
int array<T>::unshift(const T &val) {
  if (is_vector()) {
    mutate_if_vector_needed_int();

    T *it = (T *)p->int_entries;
    memmove((void *)(it + 1), it, p->int_size++ * sizeof(T));
    p->max_key++;
    new(it) T(val);
  } else {
    array_size new_size = size();
    bool is_v = (new_size.string_size == 0);

    array_inner *new_array = array_inner::create(new_size.int_size + 1, new_size.string_size, is_v);
    string_hash_entry *it = p->begin();

    if (is_v) {
      new_array->push_back_vector_value(val);
    } else {
      new_array->set_map_value(0, val, false);
    }

    while (it != p->end()) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(it->int_key, it->string_key, it->value, false);
      } else {
        if (is_v) {
          new_array->push_back_vector_value(it->value);
        } else {
          new_array->set_map_value(new_array->max_key + 1, it->value, false);
        }
      }

      it = p->next(it);
    }

    p->dispose();
    p = new_array;
  }

  return count();
}


template<class T>
bool array<T>::to_bool() const {
  return (bool)(p->int_size + p->string_size);
}

template<class T>
int array<T>::to_int() const {
  return p->int_size + p->string_size;
}

template<class T>
double array<T>::to_float() const {
  return p->int_size + p->string_size;
}

template<class T>
int array<T>::get_reference_counter() const {
  return p->ref_cnt + 1;
}

template<class T>
void array<T>::set_reference_counter_to_const() {
  // some const arrays are placed in read only memory and can't be modified
  if (p->ref_cnt != REF_CNT_FOR_CONST) {
    p->ref_cnt = REF_CNT_FOR_CONST;
  }
}

template<class T>
const T *array<T>::get_const_vector_pointer() const {
  php_assert (is_vector());
  return &(p->get_vector_value(0));
}

template<class T>
void swap(array<T> &lhs, array<T> &rhs) {
  lhs.swap(rhs);
}

template<class T>
const array<T> array_add(array<T> a1, const array<T> &a2) {
  return a1 += a2;
}


template<class T>
bool eq2(const array<T> &lhs, const array<T> &rhs) {
  if (rhs.count() != lhs.count()) {
    return false;
  }

  for (typename array<T>::const_iterator rhs_it = rhs.begin(); rhs_it != rhs.end(); ++rhs_it) {
    if (!lhs.has_key(rhs_it) || !eq2(lhs.get_value(rhs_it), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}

template<class T1, class T2>
bool eq2(const array<T1> &lhs, const array<T2> &rhs) {
  if (rhs.count() != lhs.count()) {
    return false;
  }

  for (auto rhs_it = rhs.begin(); rhs_it != rhs.end(); ++rhs_it) {
    auto key = rhs_it.get_key();
    if (!lhs.has_key(key) || !eq2(lhs.get_value(key), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}


template<class T>
bool equals(const array<T> &lhs, const array<T> &rhs) {
  if (lhs.count() != rhs.count()) {
    return false;
  }

  for (auto lhs_it = lhs.begin(), rhs_it = rhs.begin(); lhs_it != lhs.end(); ++lhs_it, ++rhs_it) {
    if (!equals(lhs_it.get_key(), rhs_it.get_key()) || !equals(lhs_it.get_value(), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}

template<class T1, class T2>
bool equals(const array<T1> &lhs, const array<T2> &rhs) {
  if (lhs.count() != rhs.count()) {
    return false;
  }

  auto rhs_it = rhs.begin();
  for (auto lhs_it = lhs.begin(); lhs_it != lhs.end(); ++lhs_it, ++rhs_it) {
    if (!equals(lhs_it.get_key(), rhs_it.get_key()) || !equals(lhs_it.get_value(), rhs_it.get_value())) {
      return false;
    }
  }

  return true;
}

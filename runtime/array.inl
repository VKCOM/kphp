// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "common/algorithms/fastmod.h"

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

array_size::array_size(int64_t int_size, bool is_vector) :
  int_size(int_size),
  is_vector(is_vector) {}

array_size::array_size(int64_t int_size, int64_t string_size, bool is_vector) :
  int_size(int_size + string_size),
  is_vector(is_vector) {}

array_size array_size::operator+(const array_size &other) const {
  return {int_size + other.int_size, 0, is_vector && other.is_vector};
}

array_size &array_size::cut(int64_t length) {
  if (int_size > length) {
    int_size = length;
  }
  return *this;
}

array_size &array_size::min(const array_size &other) {
  if (int_size > other.int_size) {
    int_size = other.int_size;
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
      const auto offset = (end - begin) >> 1;
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
  return is_int ? key_type{int_key} : key_type{string_key};
}

template<class T>
bool array<T>::is_int_key(const typename array<T>::key_type &key) {
  return key.is_int();
}

template<>
inline typename array<Unknown>::array_inner *array<Unknown>::array_inner::empty_array() {
  static array_inner_control empty_array{
    0, ExtraRefCnt::for_global_const, -1,
    {0, 0},
    0, 2,
  };
  return static_cast<array<Unknown>::array_inner *>(&empty_array);
}

template<class T>
typename array<T>::array_inner *array<T>::array_inner::empty_array() {
  return reinterpret_cast<array_inner *>(array<Unknown>::array_inner::empty_array());
}

template<class T>
uint32_t array<T>::array_inner::choose_bucket(int64_t key) const {
  uint64_t modulo_helper = fields_for_map().modulo_helper_int_buf_size;
  return fastmod::fastmod_u32(static_cast<uint32_t>(key << 2), modulo_helper, int_buf_size);
}

template<class T>
bool array<T>::array_inner::is_vector() const {
  return is_vector_internal;
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
const typename array<T>::int_hash_entry *array<T>::array_inner::begin() const {
  return (const int_hash_entry *)get_entry(last.next);
}

template<class T>
const typename array<T>::int_hash_entry *array<T>::array_inner::next(const int_hash_entry *ptr) const {
  return (const int_hash_entry *)get_entry(ptr->next);
}

template<class T>
const typename array<T>::int_hash_entry *array<T>::array_inner::prev(const int_hash_entry *ptr) const {
  return (const int_hash_entry *)get_entry(ptr->prev);
}

template<class T>
const typename array<T>::int_hash_entry *array<T>::array_inner::end() const {
  return const_cast<array<T>::array_inner *>(this)->end();
}

template<class T>
typename array<T>::int_hash_entry *array<T>::array_inner::begin() {
  return (int_hash_entry *)get_entry(end()->next);
}

template<class T>
typename array<T>::int_hash_entry *array<T>::array_inner::next(int_hash_entry *ptr) {
  return (int_hash_entry *)get_entry(ptr->next);
}

template<class T>
typename array<T>::int_hash_entry *array<T>::array_inner::prev(int_hash_entry *ptr) {
  return (int_hash_entry *)get_entry(ptr->prev);
}

template<class T>
typename array<T>::int_hash_entry *array<T>::array_inner::end() {
  return (int_hash_entry * ) &last;
}

template<class T>
bool array<T>::array_inner::is_string_hash_entry(const int_hash_entry *ptr) const {
  return !ptr->is_int;
}

template<class T>
typename array<T>::array_inner_fields_for_map &array<T>::array_inner::fields_for_map() {
  return *reinterpret_cast<array_inner_fields_for_map *>(reinterpret_cast<char *>(this) - sizeof(array_inner_fields_for_map));
}

template<class T>
const typename array<T>::array_inner_fields_for_map &array<T>::array_inner::fields_for_map() const {
  return const_cast<array_inner *>(this)->fields_for_map();
}

template<class T>
size_t array<T>::array_inner::sizeof_vector(uint32_t int_size) {
  return sizeof(array_inner) + int_size * sizeof(T);
}

template<class T>
size_t array<T>::array_inner::sizeof_map(uint32_t int_size) {
  return sizeof(array_inner_fields_for_map) + sizeof(array_inner) + int_size * sizeof(int_hash_entry);
}

template<class T>
size_t array<T>::array_inner::estimate_size(int64_t &new_int_size, bool is_vector) {
  new_int_size = std::max(new_int_size, int64_t{0});

  if (new_int_size > MAX_HASHTABLE_SIZE) {
    php_critical_error ("max array size exceeded: int_size = %" PRIi64, new_int_size);
  }

  if (is_vector) {
    new_int_size += 2;
    return sizeof_vector(static_cast<uint32_t>(new_int_size));
  }

  new_int_size = 2 * new_int_size + 3;
  if (new_int_size % 5 == 0) {
    new_int_size += 2;
  }

  return sizeof_map(static_cast<uint32_t>(new_int_size));
}

template<class T>
typename array<T>::array_inner *array<T>::array_inner::create(int64_t new_int_size, bool is_vector) {
  const size_t mem_size = estimate_size(new_int_size, is_vector);
  if (is_vector) {
    auto p = reinterpret_cast<array_inner *>(dl::allocate(mem_size));
    p->is_vector_internal = true;
    p->ref_cnt = 0;
    p->max_key = -1;
    p->int_size = 0;
    p->int_buf_size = static_cast<uint32_t>(new_int_size);
    return p;
  }

  auto shift_pointer_to_array_inner = [](void *mem) {
    return reinterpret_cast<array_inner *>(static_cast<char *>(mem) + sizeof(array_inner_fields_for_map));
  };

  array_inner *p = shift_pointer_to_array_inner(dl::allocate0(mem_size));
  p->is_vector_internal = false;
  p->ref_cnt = 0;
  p->max_key = -1;
  p->end()->next = p->get_pointer(p->end());
  p->end()->prev = p->get_pointer(p->end());

  p->int_buf_size = static_cast<uint32_t>(new_int_size);
  p->fields_for_map().modulo_helper_int_buf_size = fastmod::computeM_u32(p->int_buf_size);

  p->int_size = 0;
  return p;
}

template<class T>
void array<T>::array_inner::dispose() {
  if (ref_cnt < ExtraRefCnt::for_global_const) {
    ref_cnt--;
    if (ref_cnt <= -1) {
      if (is_vector()) {
        for (uint32_t i = 0; i < int_size; i++) {
          ((T *)int_entries)[i].~T();
        }

        dl::deallocate((void *)this, sizeof_vector(int_buf_size));
        return;
      }

      for (const int_hash_entry *it = begin(); it != end(); it = next(it)) {
        it->value.~T();
        if (is_string_hash_entry(it)) {
          it->string_key.~string();
        }
      }

      php_assert(this != empty_array());
      auto shifted_this = reinterpret_cast<char *>(this) - sizeof(array_inner_fields_for_map);
      dl::deallocate(shifted_this, sizeof_map(int_buf_size));
    }
  }
}


template<class T>
typename array<T>::array_inner *array<T>::array_inner::ref_copy() {
  if (ref_cnt < ExtraRefCnt::for_global_const) {
    ref_cnt++;
  }
  return this;
}

template<class T>
template<class ...Args>
inline T &array<T>::array_inner::emplace_back_vector_value(Args &&... args) noexcept {
  static_assert(std::is_constructible<T, Args...>{}, "should be constructible");
  php_assert (int_size < int_buf_size);
  new(&((T *)int_entries)[int_size]) T(std::forward<Args>(args)...);
  max_key++;
  int_size++;
  return reinterpret_cast<T *>(int_entries)[max_key];
}

template<class T>
T &array<T>::array_inner::push_back_vector_value(const T &v) {
  return emplace_back_vector_value(v);
}

template<class T>
T &array<T>::array_inner::get_vector_value(int64_t int_key) {
  return reinterpret_cast<T *>(int_entries)[int_key];
}

template<class T>
const T &array<T>::array_inner::get_vector_value(int64_t int_key) const {
  return reinterpret_cast<const T *>(int_entries)[int_key];
}

template<class T>
template<class ...Args>
T &array<T>::array_inner::emplace_vector_value(int64_t int_key, Args &&... args) noexcept {
  static_assert(std::is_constructible<T, Args...>{}, "should be constructible");
  reinterpret_cast<T *>(int_entries)[int_key] = T(std::forward<Args>(args)...);
  return get_vector_value(int_key);
}

template<class T>
T &array<T>::array_inner::set_vector_value(int64_t int_key, const T &v) {
  return emplace_vector_value(int_key, v);
}

template<class T>
template<class ...Args>
T &array<T>::array_inner::emplace_int_key_map_value(overwrite_element policy, int64_t int_key, Args &&... args) noexcept {
  static_assert(std::is_constructible<T, Args...>{}, "should be constructible");
  uint32_t bucket = choose_bucket(int_key);
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

    new(&int_entries[bucket].value) T(std::forward<Args>(args)...);

    int_size++;

    if (int_key > max_key) {
      max_key = int_key;
    }
  } else if (policy == overwrite_element::YES) {
    int_entries[bucket].value = T(std::forward<Args>(args)...);
  }

  return int_entries[bucket].value;
}

template<class T>
T &array<T>::array_inner::set_map_value(overwrite_element policy, int64_t int_key, const T &v) {
  return emplace_int_key_map_value(policy, int_key, v);
}

template<class T>
T array<T>::array_inner::unset_vector_value() {
  --int_size;
  T res = std::move(reinterpret_cast<T *>(int_entries)[max_key--]);
  return res;
}

template<class T>
T array<T>::array_inner::unset_map_value(int64_t int_key) {
  uint32_t bucket = choose_bucket(int_key);
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

    T res = std::move(int_entries[bucket].value);

    int_size--;

#define FIXD(a) ((a) >= int_buf_size ? (a) - int_buf_size : (a))
#define FIXU(a, m) ((a) <= (m) ? (a) + int_buf_size : (a))
    uint32_t j, rj, ri = bucket;
    for (j = bucket + 1; 1; j++) {
      rj = FIXD(j);
      if (int_entries[rj].next == EMPTY_POINTER) {
        break;
      }

      uint32_t bucket_j = choose_bucket(int_entries[rj].int_key);
      uint32_t wnt = FIXU(bucket_j, bucket);

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
    return res;
  }
  return {};
}

template<class T>
template<class S>
auto &array<T>::array_inner::find_map_entry(S &self, int64_t int_key) noexcept {
  uint32_t bucket = self.choose_bucket(int_key);
  while (self.int_entries[bucket].next != EMPTY_POINTER && self.int_entries[bucket].int_key != int_key) {
    if (unlikely (++bucket == self.int_buf_size)) {
      bucket = 0;
    }
  }

  return self.int_entries[bucket];
}

template<class T>
template<class S>
auto &array<T>::array_inner::find_map_entry(S &self, const string &string_key, int64_t precomputed_hash) noexcept {
  return find_map_entry(self, string_key.c_str(), string_key.size(), precomputed_hash);
}

template<class T>
template<class S>
auto &array<T>::array_inner::find_map_entry(S &self, const char *key, string::size_type key_size, int64_t precomputed_hash) noexcept {
  static const auto str_not_eq = [](const string &lhs, const char *rhs, string::size_type rhs_size) {
    return lhs.size() != rhs_size || string::compare(lhs, rhs, rhs_size) != 0;
  };
  auto *string_entries = self.int_entries;
  uint32_t bucket = self.choose_bucket(precomputed_hash);
  while (string_entries[bucket].next != EMPTY_POINTER &&
         (string_entries[bucket].int_key != precomputed_hash || str_not_eq(string_entries[bucket].string_key, key, key_size))) {
    if (unlikely (++bucket == self.int_buf_size)) {
      bucket = 0;
    }
  }

  return string_entries[bucket];
}

template<class T>
template<class ...Key>
const T *array<T>::array_inner::find_map_value(Key &&... key) const noexcept {
  const auto &entry = find_map_entry(*this, std::forward<Key>(key)...);
  return entry.next != EMPTY_POINTER ? &entry.value : nullptr;
}

template<class T>
const T *array<T>::array_inner::find_vector_value(int64_t int_key) const noexcept {
  return int_key >= 0 && int_key < int_size ? &get_vector_value(int_key) : nullptr;
}

template<class T>
T *array<T>::array_inner::find_vector_value(int64_t int_key) noexcept {
  return int_key >= 0 && int_key < int_size ? &get_vector_value(int_key) : nullptr;
}

template<class T>
template<class STRING, class ...Args>
std::pair<T &, bool> array<T>::array_inner::emplace_string_key_map_value(overwrite_element policy, int64_t int_key, STRING &&string_key, Args &&... args) noexcept {
  static_assert(std::is_same<std::decay_t<STRING>, string>::value, "string_key should be string");

  int_hash_entry *string_entries = int_entries;
  uint32_t bucket = choose_bucket(int_key);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == int_buf_size)) {
      bucket = 0;
    }
  }

  bool inserted = false;
  if (string_entries[bucket].next == EMPTY_POINTER) {
    string_entries[bucket].int_key = int_key;
    new(&string_entries[bucket].string_key) string{std::forward<STRING>(string_key)};

    string_entries[bucket].prev = end()->prev;
    get_entry(end()->prev)->next = get_pointer(&string_entries[bucket]);

    string_entries[bucket].next = get_pointer(end());
    end()->prev = get_pointer(&string_entries[bucket]);

    new(&string_entries[bucket].value) T(std::forward<Args>(args)...);

    int_size++;
    inserted = true;
  } else if (policy == overwrite_element::YES) {
    string_entries[bucket].value = T(std::forward<Args>(args)...);
    inserted = true;
  }

  return {string_entries[bucket].value, inserted};
}

template<class T>
T &array<T>::array_inner::set_map_value(overwrite_element policy, int64_t int_key, const string &string_key, const T &v) {
  return emplace_string_key_map_value(policy, int_key, string_key, v).first;
}

template<class T>
T array<T>::array_inner::unset_map_value(const string &string_key, int64_t precomputed_hash) {
  int_hash_entry *string_entries = int_entries;
  uint32_t bucket = choose_bucket(precomputed_hash);
  while (string_entries[bucket].next != EMPTY_POINTER && (string_entries[bucket].int_key != precomputed_hash || string_entries[bucket].string_key != string_key)) {
    if (unlikely (++bucket == int_buf_size)) {
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

    T res = std::move(string_entries[bucket].value);

    int_size--;

#define FIXD(a) ((a) >= int_buf_size ? (a) - int_buf_size : (a))
#define FIXU(a, m) ((a) <= (m) ? (a) + int_buf_size : (a))
    uint32_t j, rj, ri = bucket;
    for (j = bucket + 1; 1; j++) {
      rj = FIXD(j);
      if (string_entries[rj].next == EMPTY_POINTER) {
        break;
      }

      uint32_t bucket_j = choose_bucket(string_entries[rj].int_key);
      uint32_t wnt = FIXU(bucket_j, bucket);

      if (wnt > j || wnt <= bucket) {
        list_hash_entry *ei = string_entries + ri, *ej = string_entries + rj;
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
    return res;
  }
  return {};
}

template<class T>
bool array<T>::array_inner::is_vector_internal_or_last_index(int64_t key) const noexcept {
  return key >= 0 && key <= int_size;
}

template<class T>
size_t array<T>::array_inner::estimate_memory_usage() const {
  int64_t int_elements = int_size;
  const bool vector_structure = is_vector();
  if (vector_structure) {
    return estimate_size(int_elements, vector_structure);
  }
  return estimate_size(++int_elements, vector_structure);
}

template<class T>
bool array<T>::is_vector() const {
  return p->is_vector();
}

template<class T>
bool array<T>::is_pseudo_vector() const {
  int64_t n = 0;
  for (auto element : *this) {
    if (element.is_string_key()) {
      return false;
    }
    if (element.get_key().as_int() != n++) {
      return false;
    }
  }
  return n == count();
}

template<class T>
bool array<T>::mutate_if_vector_shared(uint32_t mul) {
  return mutate_to_size_if_vector_shared(mul * int64_t{p->int_size});
}

template<class T>
bool array<T>::mutate_to_size_if_vector_shared(int64_t int_size) {
  if (p->ref_cnt > 0) {
    array_inner *new_array = array_inner::create(int_size, true);

    const auto size = static_cast<uint32_t>(p->int_size);
    T *it = (T *)p->int_entries;

    for (uint32_t i = 0; i < size; i++) {
      new_array->push_back_vector_value(it[i]);
    }

    p->dispose();
    p = new_array;
    return true;
  }
  return false;
}

template<class T>
bool array<T>::mutate_if_map_shared(uint32_t mul) {
  if (p->ref_cnt > 0) {
    array_inner *new_array = array_inner::create(p->int_size * mul + 1, false);

    for (const int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
      } else {
        new_array->set_map_value(overwrite_element::YES, it->int_key, it->value);
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
    mutate_to_size(int64_t{p->int_buf_size} * 2);
  }
}

template<class T>
void array<T>::mutate_to_size(int64_t int_size) {
  if (mutate_to_size_if_vector_shared(int_size)) {
    return;
  }
  if (unlikely(int_size > array_inner::MAX_HASHTABLE_SIZE)) {
    php_critical_error ("max array size exceeded: int_size = %" PRIi64, int_size);
  }
  const auto new_int_buff_size = static_cast<uint32_t>(int_size);
  p = static_cast<array_inner *>(dl::reallocate(p, p->sizeof_vector(new_int_buff_size), p->sizeof_vector(p->int_buf_size)));
  p->int_buf_size = new_int_buff_size;
}

template<class T>
void array<T>::mutate_if_map_needed_int() {
  if (mutate_if_map_shared(2)) {
    return;
  }

  // not shared (ref_cnt == 0)
  if (p->int_size * 5 > 3 * p->int_buf_size) {
    int64_t new_int_size = p->int_size * 2 + 1;
    array_inner *new_array = array_inner::create(new_int_size, false);

    for (int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        new_array->emplace_string_key_map_value(overwrite_element::YES,
                                                it->int_key, std::move(it->string_key), std::move(it->value));
      } else {
        new_array->emplace_int_key_map_value(overwrite_element::YES,
                                             it->int_key, std::move(it->value));
      }
    }

    p->dispose();
    p = new_array;
  }
}

template<class T>
void array<T>::mutate_to_map_if_vector_or_map_need_string() {
  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_needed_int();
  }
}

template<class T>
void array<T>::reserve(int64_t int_size, int64_t string_size, bool make_vector_if_possible) {
  reserve(int_size + string_size, make_vector_if_possible);
}

template<class T>
void array<T>::reserve(int64_t int_size, bool make_vector_if_possible) {
  if (int_size > int64_t{p->int_buf_size}) {
    if (is_vector() && make_vector_if_possible) {
      mutate_to_size(int_size);
    } else {
      const int64_t new_int_size = std::max(int_size, int64_t{p->int_buf_size});
      array_inner *new_array = array_inner::create(new_int_size, false);

      if (is_vector()) {
        for (uint32_t it = 0; it != p->int_size; it++) {
          new_array->set_map_value(overwrite_element::YES, it, ((T *)p->int_entries)[it]);
        }
        php_assert (new_array->max_key == p->max_key);
      } else {
        for (const int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
          if (p->is_string_hash_entry(it)) {
            new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
          } else {
            new_array->set_map_value(overwrite_element::YES, it->int_key, it->value);
          }
        }
      }

      p->dispose();
      p = new_array;
    }
  }
}

template<class T>
size_t array<T>::estimate_memory_usage() const {
  return p->estimate_memory_usage();
}

template<class T>
typename array<T>::const_iterator array<T>::cbegin() const {
  return const_iterator::make_begin(*this);
}

template<class T>
typename array<T>::const_iterator array<T>::begin() const {
  return const_iterator::make_begin(*this);
}

template<class T>
typename array<T>::const_iterator array<T>::middle(int64_t n) const {
  return const_iterator::make_middle(*this, n);
}

template<class T>
typename array<T>::const_iterator array<T>::cend() const {
  return const_iterator::make_end(*this);
}

template<class T>
typename array<T>::const_iterator array<T>::end() const {
  return cend();
}

template<class T>
typename array<T>::iterator array<T>::begin() {
  return iterator::make_begin(*this);
}

template<class T>
typename array<T>::iterator array<T>::middle(int64_t n) {
  return iterator::make_middle(*this, n);
}

template<class T>
typename array<T>::iterator array<T>::end() {
  return iterator::make_end(*this);
}


template<class T>
void array<T>::convert_to_map() {
  array_inner *new_array = array_inner::create(p->int_size + 4, false);

  T *elements = reinterpret_cast<T *>(p->int_entries);
  const bool move_values = p->ref_cnt == 0;
  if (move_values) {
    for (uint32_t it = 0; it != p->int_size; it++) {
      new_array->emplace_int_key_map_value(overwrite_element::YES, it, std::move(elements[it]));
    }
  } else {
    for (uint32_t it = 0; it != p->int_size; it++) {
      new_array->set_map_value(overwrite_element::YES, it, elements[it]);
    }
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

  array_inner *new_array = array_inner::create(other.p->int_size, other.is_vector());

  if (new_array->is_vector()) {
    uint32_t size = other.p->int_size;
    T1 *it = reinterpret_cast<T1 *>(other.p->int_entries);
    for (uint32_t i = 0; i < size; i++) {
      new_array->push_back_vector_value(convert_to<T>::convert(it[i]));
    }
  } else {
    for (const typename array<T1>::int_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, convert_to<T>::convert(it->value));
      } else {
        new_array->set_map_value(overwrite_element::YES, it->int_key, convert_to<T>::convert(it->value));
      }
    }
  }

  p = new_array;

  php_assert (new_array->int_size == other.p->int_size);
}

template<class T>
template<class T1>
void array<T>::move_from(array<T1> &&other) noexcept {
  if (other.empty()) {
    p = array_inner::empty_array();
    return;
  }

  if (other.p->ref_cnt > 0) {
    copy_from(other);
    other = array<T1>{};
    return;
  }

  array_inner *new_array = array_inner::create(other.p->int_size, other.is_vector());

  if (new_array->is_vector()) {
    uint32_t size = other.p->int_size;
    T1 *it = reinterpret_cast<T1 *>(other.p->int_entries);
    for (uint32_t i = 0; i < size; i++) {
      new_array->emplace_back_vector_value(convert_to<T>::convert(std::move(it[i])));
    }
  } else {
    for (auto it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        new_array->emplace_string_key_map_value(overwrite_element::YES, it->int_key,
                                                std::move(it->string_key), convert_to<T>::convert(std::move(it->value)));
      } else {
        new_array->emplace_int_key_map_value(overwrite_element::YES,
                                             it->int_key, convert_to<T>::convert(std::move(it->value)));
      }
    }
  }

  p = new_array;
  php_assert (new_array->int_size == other.p->int_size);

  other = array<T1>{};
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
  p(array_inner::create(s.int_size, s.is_vector)) {
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
array<T>::array(const array<T> &other) noexcept :
  p(other.p->ref_copy()) {}

template<class T>
array<T>::array(array<T> &&other) noexcept :
  p(other.p) {
  other.p = array_inner::empty_array();
}

template<class T>
template<class T1, class>
array<T>::array(const array<T1> &other) noexcept {
  copy_from(other);
}

template<class T>
template<class T1, class>
array<T>::array(array<T1> &&other) noexcept {
  move_from(std::move(other));
}

template<class T>
template<class... Args>
inline array<T> array<T>::create(Args &&... args) {
  static_assert((std::is_convertible<std::decay_t<Args>, T>::value && ...), "Args type must be convertible to T");

  array<T> res{array_size{sizeof...(args), 0, true}};
  (res.p->emplace_back_vector_value(std::forward<Args>(args)), ...);
  return res;
}

template<class T>
array<T> &array<T>::operator=(const array &other) noexcept {
  auto other_copy = other.p->ref_copy();
  destroy();
  p = other_copy;
  return *this;
}

template<class T>
array<T> &array<T>::operator=(array &&other) noexcept {
  if (this != &other) {
    destroy();
    p = other.p;
    other.p = array_inner::empty_array();
  }
  return *this;
}

template<class T>
template<class T1, class>
array<T> &array<T>::operator=(const array<T1> &other) noexcept {
  destroy();
  copy_from(other);
  return *this;
}

template<class T>
template<class T1, class>
array<T> &array<T>::operator=(array<T1> &&other) noexcept {
  destroy();
  move_from(std::move(other));
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
T &array<T>::operator[](int64_t int_key) {
  if (is_vector()) {
    if (p->is_vector_internal_or_last_index(int_key)) {
      if (int_key == p->int_size) {
        mutate_if_vector_needed_int();
        return p->emplace_back_vector_value();
      } else {
        mutate_if_vector_shared();
        return p->get_vector_value(int_key);
      }
    }

    convert_to_map();
  } else {
    mutate_if_map_needed_int();
  }

  return p->emplace_int_key_map_value(overwrite_element::NO, int_key);
}

template<class T>
T &array<T>::operator[](const string &string_key) {
  int64_t int_val = 0;
  if (string_key.try_to_int(&int_val)) {
    return (*this)[int_val];
  }

  mutate_to_map_if_vector_or_map_need_string();
  return p->emplace_string_key_map_value(overwrite_element::NO, string_key.hash(), string_key).first;
}

template<class T>
T &array<T>::operator[](tmp_string string_key) {
  // TODO: try not to allocate here too
  return (*this)[materialize_tmp_string(string_key)];
}

template<class T>
T &array<T>::operator[](const mixed &v) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return (*this)[string()];
    case mixed::type::BOOLEAN:
      return (*this)[static_cast<int64_t>(v.as_bool())];
    case mixed::type::INTEGER:
      return (*this)[v.as_int()];
    case mixed::type::FLOAT:
      return (*this)[static_cast<int64_t>(v.as_double())];
    case mixed::type::STRING:
      return (*this)[v.as_string()];
    case mixed::type::ARRAY:
      php_warning("Illegal offset type array");
      return (*this)[v.as_array().to_int()];
    default:
      __builtin_unreachable();
  }
}

template<class T>
T &array<T>::operator[](double double_key) {
  return (*this)[static_cast<int64_t>(double_key)];
}

template<class T>
T &array<T>::operator[](const const_iterator &it) noexcept {
  if (it.self_->is_vector()) {
    const auto key = static_cast<int64_t>(reinterpret_cast<const T *>(it.entry_) - reinterpret_cast<const T *>(it.self_->int_entries));
    return operator[](key);
  }
  auto *entry = reinterpret_cast<const int_hash_entry *>(it.entry_);
  if (it.self_->is_string_hash_entry(entry)) {
    mutate_to_map_if_vector_or_map_need_string();
    return p->emplace_string_key_map_value(overwrite_element::NO, entry->int_key, entry->string_key).first;
  }
  return operator[](entry->int_key);
}

template<class T>
T &array<T>::operator[](const iterator &it) noexcept {
  return operator[](const_iterator{it.self_, it.entry_});
}

template<class T>
template<class ...Args>
void array<T>::emplace_value(int64_t int_key, Args &&... args) noexcept {
  if (is_vector()) {
    if (p->is_vector_internal_or_last_index(int_key)) {
      if (int_key == p->int_size) {
        mutate_if_vector_needed_int();
        p->emplace_back_vector_value(std::forward<Args>(args)...);
      } else {
        mutate_if_vector_shared();
        p->emplace_vector_value(int_key, std::forward<Args>(args)...);
      }
      return;
    }

    convert_to_map();
  } else {
    mutate_if_map_needed_int();
  }

  p->emplace_int_key_map_value(overwrite_element::YES, int_key, std::forward<Args>(args)...);
}

template<class T>
void array<T>::set_value(int64_t int_key, T &&v) noexcept {
  emplace_value(int_key, std::move(v));
}

template<class T>
void array<T>::set_value(int64_t int_key, const T &v) noexcept {
  emplace_value(int_key, v);
}

template<class T>
void array<T>::set_value(double double_key, T &&v) noexcept {
  emplace_value(static_cast<int64_t>(double_key), std::move(v));
}

template<class T>
void array<T>::set_value(double double_key, const T &v) noexcept {
  emplace_value(static_cast<int64_t>(double_key), v);
}

template<class T>
template<class ...Args>
void array<T>::emplace_value(const string &string_key, Args &&... args) noexcept {
  int64_t int_val = 0;
  if (string_key.try_to_int(&int_val)) {
    emplace_value(int_val, std::forward<Args>(args)...);
    return;
  }

  mutate_to_map_if_vector_or_map_need_string();
  p->emplace_string_key_map_value(overwrite_element::YES,
                                  string_key.hash(), string_key, std::forward<Args>(args)...);
}

template<class T>
void array<T>::set_value(const string &string_key, T &&v) noexcept {
  emplace_value(string_key, std::move(v));
}

template<class T>
void array<T>::set_value(const string &string_key, const T &v) noexcept {
  emplace_value(string_key, v);
}

template<class T>
void array<T>::set_value(tmp_string string_key, T &&v) noexcept {
  // TODO: rework value insertion, so tmp_string is converted to a real string
  // only when new insertion happens
  emplace_value(materialize_tmp_string(string_key), std::move(v));
}

template<class T>
void array<T>::set_value(tmp_string string_key, const T &v) noexcept {
  // TODO: rework value insertion, so tmp_string is converted to a real string
  // only when new insertion happens
  emplace_value(materialize_tmp_string(string_key), v);
}

template<class T>
void array<T>::set_value(const string &string_key, const T &v, int64_t precomputed_hash) noexcept {
  mutate_to_map_if_vector_or_map_need_string();
  p->emplace_string_key_map_value(overwrite_element::YES, precomputed_hash, string_key, v);
}

template<class T>
void array<T>::set_value(const string &string_key, T &&v, int64_t precomputed_hash) noexcept {
  mutate_to_map_if_vector_or_map_need_string();
  p->emplace_string_key_map_value(overwrite_element::YES, precomputed_hash, string_key, std::move(v));
}


template<class T>
template<class ...Args>
void array<T>::emplace_value(const mixed &var_key, Args &&... args) noexcept {
  switch (var_key.get_type()) {
    case mixed::type::NUL:
      return emplace_value(string(), std::forward<Args>(args)...);
    case mixed::type::BOOLEAN:
      return emplace_value(static_cast<int64_t>(var_key.as_bool()), std::forward<Args>(args)...);
    case mixed::type::INTEGER:
      return emplace_value(var_key.as_int(), std::forward<Args>(args)...);
    case mixed::type::FLOAT:
      return emplace_value(static_cast<int64_t>(var_key.as_double()), std::forward<Args>(args)...);
    case mixed::type::STRING:
      return emplace_value(var_key.as_string(), std::forward<Args>(args)...);
    case mixed::type::ARRAY:
      php_warning("Illegal offset type array");
      return emplace_value(var_key.as_array().to_int(), std::forward<Args>(args)...);
    default:
      __builtin_unreachable();
  }
}

template<class T>
void array<T>::set_value(const mixed &v, T &&value) noexcept {
  emplace_value(v, std::move(value));
}

template<class T>
void array<T>::set_value(const mixed &v, const T &value) noexcept {
  emplace_value(v, value);
}

template<class T>
template<class OptionalT, class ...Args>
void array<T>::emplace_value(const Optional<OptionalT> &key, Args &&... args) noexcept {
  auto set_value_lambda = [this](auto &&... args) { return this->set_value(std::forward<decltype(args)>(args)...);};
  return call_fun_on_optional_value(set_value_lambda, key, std::forward<Args>(args)...);
}

template<class T>
template<class OptionalT>
void array<T>::set_value(const Optional<OptionalT> &key, T &&value) noexcept {
  emplace_value(key, std::move(value));
}

template<class T>
template<class OptionalT>
void array<T>::set_value(const Optional<OptionalT> &key, const T &value) noexcept {
  emplace_value(key, value);
}

template<class T>
void array<T>::set_value(const const_iterator &it) noexcept {
  if (it.self_->is_vector()) {
    const auto key = static_cast<int64_t>(reinterpret_cast<const T *>(it.entry_) - reinterpret_cast<const T *>(it.self_->int_entries));
    emplace_value(key, *reinterpret_cast<const T *>(it.entry_));
    return;
  }
  auto *entry = reinterpret_cast<const int_hash_entry *>(it.entry_);
  if (it.self_->is_string_hash_entry(entry)) {
    mutate_to_map_if_vector_or_map_need_string();
    p->emplace_string_key_map_value(overwrite_element::YES, entry->int_key, entry->string_key, entry->value);
  } else {
    emplace_value(entry->int_key, entry->value);
  }
}

template<class T>
void array<T>::set_value(const iterator &it) noexcept {
  set_value(const_iterator{it.self_, it.entry_});
}

template<class T>
void array<T>::assign_raw(const char *s) {
  p = reinterpret_cast<array_inner *>(const_cast<char *>(s));
}

template<class T>
const T *array<T>::find_value(int64_t int_key) const noexcept {
  return p->is_vector()
         ? p->find_vector_value(int_key)
         : p->find_map_value(int_key);
}

template<class T>
const T *array<T>::find_value(const char *s, string::size_type l) const noexcept {
  int64_t int_val = 0;
  const bool is_key_int = php_try_to_int(s, l, &int_val);
  if (p->is_vector()) {
    return is_key_int ? p->find_vector_value(int_val) : nullptr;
  }
  return is_key_int ? p->find_map_value(int_val) : p->find_map_value(s, l, string_hash(s, l));
}

template<class T>
const T *array<T>::find_value(const string &string_key, int64_t precomputed_hash) const noexcept {
  return p->is_vector() ? nullptr : p->find_map_value(string_key, precomputed_hash);
}

template<class T>
const T *array<T>::find_value(const mixed &v) const noexcept {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return find_value(string());
    case mixed::type::BOOLEAN:
      return find_value(static_cast<int64_t>(v.as_bool()));
    case mixed::type::INTEGER:
      return find_value(v.as_int());
    case mixed::type::FLOAT:
      return find_value(static_cast<int64_t>(v.as_double()));
    case mixed::type::STRING:
      return find_value(v.as_string());
    case mixed::type::ARRAY:
      php_warning("Illegal offset type array");
      return find_value(v.as_array().to_int());
    default:
      __builtin_unreachable();
  }
}

template<class T>
const T *array<T>::find_value(double double_key) const noexcept {
  return find_value(static_cast<int64_t>(double_key));
}

template<class T>
const T *array<T>::find_value(const const_iterator &it) const noexcept {
  if (it.self_->is_vector()) {
    const auto key = static_cast<int64_t>(reinterpret_cast<const T *>(it.entry_) - reinterpret_cast<const T *>(it.self_->int_entries));
    return find_value(key);
  } else {
    auto *entry = reinterpret_cast<const int_hash_entry *>(it.entry_);
    return it.self_->is_string_hash_entry(entry)
           ? find_value(entry->string_key, entry->int_key)
           : find_value(entry->int_key);
  }
}

template<class T>
const T *array<T>::find_value(const iterator &it) const noexcept {
  return find_value(const_iterator{it.self_, it.entry_});
}

template<class T>
typename array<T>::iterator array<T>::find_no_mutate(int64_t int_key) noexcept {
  if (p->is_vector()) {
    if (auto *vector_entry = p->find_vector_value(int_key)) {
      return iterator{p, reinterpret_cast<list_hash_entry *>(vector_entry)};
    }
    return end_no_mutate();
  }
  return find_iterator_in_map_no_mutate(int_key);
}

template<class T>
typename array<T>::iterator array<T>::find_no_mutate(const string &string_key) noexcept {
  int64_t int_key = 0;
  if (string_key.try_to_int(&int_key)) {
    return find_no_mutate(int_key);
  }
  if (p->is_vector()) {
    return end_no_mutate();
  }
  return find_iterator_in_map_no_mutate(string_key, string_key.hash());
}

template<class T>
template<class ...Key>
typename array<T>::iterator array<T>::find_iterator_in_map_no_mutate(const Key &... key) noexcept {
  list_hash_entry &map_entry = array_inner::find_map_entry(*p, key...);
  if (map_entry.next != array_inner::EMPTY_POINTER) {
    return iterator{p, &map_entry};
  }
  return end_no_mutate();
}

template<class T>
typename array<T>::iterator array<T>::find_no_mutate(const mixed &v) noexcept {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return find_no_mutate(string());
    case mixed::type::BOOLEAN:
      return find_no_mutate(static_cast<int64_t>(v.as_bool()));
    case mixed::type::INTEGER:
      return find_no_mutate(v.as_int());
    case mixed::type::FLOAT:
      return find_no_mutate(static_cast<int64_t>(v.as_double()));
    case mixed::type::STRING:
      return find_no_mutate(v.as_string());
    case mixed::type::ARRAY:
      php_warning("Illegal offset type array");
      return find_no_mutate(v.as_array().to_int());
    default:
      __builtin_unreachable();
  }
}

template<class T>
template<class K>
const mixed array<T>::get_var(const K &key) const {
  auto *value = find_value(key);
  return value ? mixed{*value} : mixed{};
}

template<class T>
template<class K>
const T array<T>::get_value(const K &key) const {
  auto *value = find_value(key);
  return value ? *value : T{};
}

template<class T>
const T array<T>::get_value(const string &string_key, int64_t precomputed_hash) const {
  auto *value = find_value(string_key, precomputed_hash);
  return value ? *value : T{};
}

template<class T>
template<class K>
bool array<T>::has_key(const K &key) const {
  return find_value(key) != nullptr;
}

template<class T>
template<class K>
bool array<T>::isset(const K &key) const noexcept {
  auto *value = find_value(key);
  return value && !f$is_null(*value);
}

template<class T>
bool array<T>::isset(const string &key, int64_t precomputed_hash) const noexcept {
  auto *value = find_value(key, precomputed_hash);
  return value && !f$is_null(*value);
}

template<class T>
T array<T>::unset(int64_t int_key) {
  if (is_vector()) {
    if (int_key < 0 || int_key >= p->int_size) {
      return {};
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
T array<T>::unset(const string &string_key) {
  int64_t int_val = 0;
  if (string_key.try_to_int(&int_val)) {
    return unset(int_val);
  }

  if (is_vector()) {
    return {};
  }

  return unset(string_key, string_key.hash());
}

template<class T>
T array<T>::unset(const string &string_key, int64_t precomputed_hash) {
  if (is_vector()) {
    return {};
  }

  mutate_if_map_shared();
  return p->unset_map_value(string_key, precomputed_hash);
}

template<class T>
T array<T>::unset(const mixed &v) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      return unset(string());
    case mixed::type::BOOLEAN:
      return unset(static_cast<int64_t>(v.as_bool()));
    case mixed::type::INTEGER:
      return unset(v.as_int());
    case mixed::type::FLOAT:
      return unset(static_cast<int64_t>(v.as_double()));
    case mixed::type::STRING:
      return unset(v.as_string());
    case mixed::type::ARRAY:
      php_warning("Illegal offset type array");
      return unset(v.as_array().to_int());
    default:
      __builtin_unreachable();
  }
}

template<class T>
bool array<T>::empty() const {
  return count() == 0;
}

template<class T>
int64_t array<T>::count() const {
  return p->int_size;
}

template<class T>
array_size array<T>::size() const {
  return {p->int_size, is_vector()};
}

template<class T>
template<class T1, class>
void array<T>::merge_with(const array<T1> &other) noexcept {
  for (auto it : other) {
    push_back_iterator(it);
  }
}

template<class T>
template<class T1, class>
void array<T>::merge_with_recursive(const array<T1> &other) noexcept {
  for (auto it : other) {
    push_back_iterator<merge_recursive::YES>(it);
  }
}

template<class T>
void array<T>::merge_with_recursive(const mixed &other) noexcept {
  if (other.is_array()) {
    return merge_with_recursive(other.as_array());
  }
  if constexpr (std::is_convertible_v<mixed, T>) {
    merge_with_recursive(array<T>::create(other));
  } else {
    php_warning("Array merge isn't possible: can't convert mixed to array item type");
  }
}

template<class T>
const array<T> array<T>::operator+(const array<T> &other) const {
  array<T> result(size() + other.size());

  if (is_vector()) {
    uint32_t size = p->int_size;
    T *it = (T *)p->int_entries;

    if (result.is_vector()) {
      for (uint32_t i = 0; i < size; i++) {
        result.p->push_back_vector_value(it[i]);
      }
    } else {
      for (uint32_t i = 0; i < size; i++) {
        result.p->set_map_value(overwrite_element::YES, i, it[i]);
      }
    }
  } else {
    for (const int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
      if (p->is_string_hash_entry(it)) {
        result.p->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
      } else {
        result.p->set_map_value(overwrite_element::YES, it->int_key, it->value);
      }
    }
  }

  if (other.is_vector()) {
    uint32_t size = other.p->int_size;
    T *it = (T *)other.p->int_entries;

    if (result.is_vector()) {
      for (uint32_t i = p->int_size; i < size; i++) {
        result.p->push_back_vector_value(it[i]);
      }
    } else {
      for (uint32_t i = 0; i < size; i++) {
        result.p->set_map_value(overwrite_element::NO, i, it[i]);
      }
    }
  } else {
    for (const int_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        result.p->set_map_value(overwrite_element::NO, it->int_key, it->string_key, it->value);
      } else {
        result.p->set_map_value(overwrite_element::NO, it->int_key, it->value);
      }
    }
  }

  return result;
}

template<class T>
array<T> &array<T>::operator+=(const array<T> &other) {
  if (other.empty()) {
    return *this;
  }
  if (is_vector()) {
    if (other.is_vector()) {
      uint32_t size = other.p->int_size;
      T *it = (T *)other.p->int_entries;

      if (p->ref_cnt > 0) {
        uint32_t my_size = p->int_size;
        T *my_it = (T *)p->int_entries;

        array_inner *new_array = array_inner::create(max(size, my_size), true);

        for (uint32_t i = 0; i < my_size; i++) {
          new_array->push_back_vector_value(my_it[i]);
        }

        p->dispose();
        p = new_array;
      } else if (p->int_buf_size < size + 2) {
        uint32_t new_size = max(size + 2, p->int_buf_size * 2);
        p = (array_inner *)dl::reallocate((void *)p, p->sizeof_vector(new_size), p->sizeof_vector(p->int_buf_size));
        p->int_buf_size = new_size;
      }

      if (p->int_size > 0 && size > 0) {
        php_warning("Strange usage of array operator += on two vectors. Did you mean array_merge?");
      }

      for (uint32_t i = p->int_size; i < size; i++) {
        p->push_back_vector_value(it[i]);
      }

      return *this;
    } else {
      array_inner *new_array = array_inner::create(p->int_size + other.p->int_size + 4, false);
      T *it = (T *)p->int_entries;

      for (uint32_t i = 0; i != p->int_size; i++) {
        new_array->set_map_value(overwrite_element::YES, i, it[i]);
      }

      p->dispose();
      p = new_array;
    }
  } else {
    if (p == other.p) {
      return *this;
    }

    uint32_t new_int_size = p->int_size + other.p->int_size;

    if (new_int_size * 5 > 3 * p->int_buf_size || p->ref_cnt > 0) {
      array_inner *new_array = array_inner::create(max(new_int_size, 2 * p->int_size) + 1, false);

      for (const int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
        if (p->is_string_hash_entry(it)) {
          new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
        } else {
          new_array->set_map_value(overwrite_element::YES, it->int_key, it->value);
        }
      }

      p->dispose();
      p = new_array;
    }
  }

  if (other.is_vector()) {
    uint32_t size = other.p->int_size;
    T *it = (T *)other.p->int_entries;

    for (uint32_t i = 0; i < size; i++) {
      p->set_map_value(overwrite_element::NO, i, it[i]);
    }
  } else {
    for (int_hash_entry *it = other.p->begin(); it != other.p->end(); it = other.p->next(it)) {
      if (other.p->is_string_hash_entry(it)) {
        p->set_map_value(overwrite_element::NO, it->int_key, it->string_key, it->value);
      } else {
        p->set_map_value(overwrite_element::NO, it->int_key, it->value);
      }
    }
  }

  return *this;
}

template<class T>
template<class ...Args>
T &array<T>::emplace_back(Args &&... args) noexcept {
  if (is_vector()) {
    mutate_if_vector_needed_int();
    return p->emplace_back_vector_value(std::forward<Args>(args)...);
  } else {
    mutate_if_map_needed_int();
    return p->emplace_int_key_map_value(overwrite_element::YES, get_next_key(), std::forward<Args>(args)...);
  }
}

template<class T>
void array<T>::push_back(T &&v) noexcept {
  emplace_back(std::move(v));
}

template<class T>
void array<T>::push_back(const T &v) noexcept {
  emplace_back(v);
}

template<class T>
template<merge_recursive recursive>
std::enable_if_t<recursive == merge_recursive::YES> array<T>::start_merge_recursive(T &value, bool was_inserted, const T &other_value) noexcept {
  static_assert(std::is_same_v<T, mixed>);
  if (!was_inserted) {
    if (!value.is_array()) {
      value = array<T>::create(value);
    }
    value.as_array().merge_with_recursive(other_value);
  }
}

template<class T>
template<merge_recursive recursive, class T1>
void array<T>::push_back_iterator(const array_iterator<T1> &it) noexcept {
  if (it.self_->is_vector()) {
    emplace_back(*reinterpret_cast<const T1 *>(it.entry_));
  } else {
    auto *entry = reinterpret_cast<typename array_iterator<T1>::int_hash_type *>(it.entry_);
    if (it.self_->is_string_hash_entry(entry)) {
      mutate_to_map_if_vector_or_map_need_string();

      // don't overwrite existing element if we are in merge_recursive::YES mode,
      // this case will be handled further
      auto [value_ref, inserted] = p->emplace_string_key_map_value(
        recursive == merge_recursive::YES ? overwrite_element::NO : overwrite_element::YES,
        entry->int_key, entry->string_key, entry->value);

      static_assert(std::is_same_v<decltype(value_ref), T &>, "value_ref should be reference type");

      start_merge_recursive<recursive>(value_ref, inserted, entry->value);
    } else {
      if (is_vector()) {
        mutate_if_vector_needed_int();
        p->push_back_vector_value(entry->value);
      } else {
        mutate_if_map_needed_int();
        p->set_map_value(overwrite_element::YES, get_next_key(), entry->value);
      }
    }
  }
}

template<class T>
void array<T>::push_back(const const_iterator &it) noexcept {
  push_back_iterator(it);
}

template<class T>
void array<T>::push_back(const iterator &it) noexcept {
  push_back_iterator(it);
}

template<class T>
const T array<T>::push_back_return(const T &v) {
  return emplace_back(v);
}

template<class T>
const T array<T>::push_back_return(T &&v) {
  return emplace_back(std::move(v));
}

template<class T>
void array<T>::swap_int_keys(int64_t idx1, int64_t idx2) noexcept {
  if (idx1 == idx2) {
    return;
  }

  // this function is supposed to be used for vector optimization, else branch is just to be on the safe side
  if (is_vector() && idx1 >= 0 && idx2 >= 0 && idx1 < p->int_size && idx2 < p->int_size) {
    mutate_if_vector_shared();
    std::swap(reinterpret_cast<T *>(p->int_entries)[idx1], reinterpret_cast<T *>(p->int_entries)[idx2]);
  } else {
    if (auto *v1 = find_value(idx1)) {
      if (auto *v2 = find_value(idx2)) {
        T tmp = std::move(*v1);
        set_value(idx1, std::move(*v2));
        set_value(idx2, std::move(tmp));
      }
    }
  }
}

template<class T>
void array<T>::fill_vector(int64_t num, const T &value) {
  php_assert(is_vector() && p->int_size == 0 && num <= p->int_buf_size);

  std::uninitialized_fill((T *)p->int_entries, (T *)p->int_entries + num, value);
  p->max_key = num - 1;
  p->int_size = static_cast<uint32_t>(num);
}

template<class T>
void array<T>::memcpy_vector(int64_t num __attribute__((unused)), const void *src_buf __attribute__((unused))) {
  if constexpr (std::is_trivially_copyable_v<T>) {
    php_assert(is_vector() && p->int_size == 0 && num <= p->int_buf_size);
    mutate_if_vector_shared();

    memcpy(p->int_entries, src_buf, num * sizeof(T));
    p->max_key = num - 1;
    p->int_size = static_cast<uint32_t>(num);
  } else {
    php_critical_error("Can't memcpy array with not trivially copyable items");
  }
}


template<class T>
int64_t array<T>::get_next_key() const {
  return p->max_key + 1;
}


template<class T>
template<class T1>
void array<T>::sort(const T1 &compare, bool renumber) {
  int64_t n = count();

  if (renumber) {
    if (n == 0) {
      return;
    }

    if (!is_vector()) {
      array_inner *res = array_inner::create(n, true);
      for (int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
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

  int_hash_entry **arTmp = (int_hash_entry **)dl::allocate(n * sizeof(int_hash_entry * ));
  uint32_t i = 0;
  for (int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
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
  for (uint32_t j = 1; j < n; j++) {
    arTmp[j]->prev = p->get_pointer(arTmp[j - 1]);
    arTmp[j - 1]->next = p->get_pointer(arTmp[j]);
  }
  arTmp[n - 1]->next = p->get_pointer(p->end());
  p->end()->prev = p->get_pointer(arTmp[n - 1]);

  dl::deallocate(arTmp, n * sizeof(int_hash_entry * ));
}


template<class T>
template<class T1>
void array<T>::ksort(const T1 &compare) {
  int64_t n = count();
  if (n <= 1) {
    return;
  }

  if (is_vector()) {
    convert_to_map();
  } else {
    mutate_if_map_shared();
  }

  array<key_type> keys(array_size(n, 0, true));
  for (int_hash_entry *it = p->begin(); it != p->end(); it = p->next(it)) {
    if (p->is_string_hash_entry(it)) {
      keys.p->push_back_vector_value(it->get_key());
    } else {
      keys.p->push_back_vector_value(((int_hash_entry *)it)->get_key());
    }
  }

  key_type *keysp = (key_type *)keys.p->int_entries;
  dl::sort<key_type, T1>(keysp, keysp + n, compare);

  list_hash_entry *prev = (list_hash_entry *)p->end();
  for (uint32_t j = 0; j < n; j++) {
    list_hash_entry *cur;
    if (is_int_key(keysp[j])) {
      int64_t int_key = keysp[j].to_int();
      uint32_t bucket = p->choose_bucket(int_key);
      while (p->int_entries[bucket].int_key != int_key) {
        if (unlikely (++bucket == p->int_buf_size)) {
          bucket = 0;
        }
      }
      cur = (list_hash_entry * ) & p->int_entries[bucket];
    } else {
      string string_key = keysp[j].to_string();
      int64_t int_key = string_key.hash();
      int_hash_entry *string_entries = p->int_entries;
      uint32_t bucket = p->choose_bucket(int_key);
      while ((string_entries[bucket].int_key != int_key || string_entries[bucket].string_key != string_key)) {
        if (unlikely (++bucket == p->int_buf_size)) {
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
    return {};
  }

  if (is_vector()) {
    mutate_if_vector_shared();
    return p->unset_vector_value();
  }

  mutate_if_map_shared();
  int_hash_entry *it = p->prev(p->end());

  return p->is_string_hash_entry(it) ?
    p->unset_map_value(it->string_key, it->int_key) :
    p->unset_map_value(it->int_key);
}

template<class T>
T &array<T>::back() {
  return (--end()).get_value();
}

template<class T>
T array<T>::shift() {
  if (count() == 0) {
    php_warning("Cannot use array_shift on empty array");
    return T{};
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
    //TODO: check is it ok?
    bool is_v = new_size.is_vector;

    array_inner *new_array = array_inner::create(new_size.int_size, is_v);
    int_hash_entry *it = p->begin();
    T res = it->value;

    it = p->next(it);
    while (it != p->end()) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
      } else {
        if (is_v) {
          new_array->push_back_vector_value(it->value);
        } else {
          new_array->set_map_value(overwrite_element::YES, new_array->max_key + 1, it->value);
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
int64_t array<T>::unshift(const T &val) {
  if (is_vector()) {
    mutate_if_vector_needed_int();

    T *it = (T *)p->int_entries;
    memmove((void *)(it + 1), it, p->int_size++ * sizeof(T));
    p->max_key++;
    new(it) T(val);
  } else {
    array_size new_size = size();
    //TODO: check is it ok?
    bool is_v = new_size.is_vector;

    array_inner *new_array = array_inner::create(new_size.int_size + 1, is_v);
    int_hash_entry *it = p->begin();

    if (is_v) {
      new_array->push_back_vector_value(val);
    } else {
      new_array->set_map_value(overwrite_element::YES, 0, val);
    }

    while (it != p->end()) {
      if (p->is_string_hash_entry(it)) {
        new_array->set_map_value(overwrite_element::YES, it->int_key, it->string_key, it->value);
      } else {
        if (is_v) {
          new_array->push_back_vector_value(it->value);
        } else {
          new_array->set_map_value(overwrite_element::YES, new_array->max_key + 1, it->value);
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
  return static_cast<bool>(count());
}

template<class T>
int64_t array<T>::to_int() const {
  return count();
}

template<class T>
double array<T>::to_float() const {
  return static_cast<double>(count());
}

template<class T>
int64_t array<T>::get_reference_counter() const {
  return p->ref_cnt + 1;
}

template<class T>
bool array<T>::is_reference_counter(ExtraRefCnt ref_cnt_value) const noexcept {
  return p->ref_cnt == ref_cnt_value;
}

template<class T>
void array<T>::set_reference_counter_to(ExtraRefCnt ref_cnt_value) noexcept {
  // some const arrays are placed in read only memory and can't be modified
  if (p->ref_cnt != ref_cnt_value) {
    p->ref_cnt = ref_cnt_value;
  }
}

template<class T>
void array<T>::force_destroy(ExtraRefCnt expected_ref_cnt) noexcept {
  php_assert(expected_ref_cnt != ExtraRefCnt::for_global_const);
  if (p) {
    php_assert(p->ref_cnt == expected_ref_cnt);
    p->ref_cnt = 0;
    p->dispose();
    p = nullptr;
  }
}

template<class T>
typename array<T>::iterator array<T>::begin_no_mutate() {
  if (is_vector()) {
    return typename array<T>::iterator(p, p->int_entries);
  }
  return typename array<T>::iterator(p, p->begin());
}

template<class T>
typename array<T>::iterator array<T>::end_no_mutate() {
  return end();
}

template<class T>
void array<T>::mutate_if_shared() noexcept {
  if (is_vector()) {
    mutate_if_vector_shared();
  } else {
    mutate_if_map_shared();
  }
}

template<class T>
const T *array<T>::get_const_vector_pointer() const {
  php_assert (is_vector());
  return &(p->get_vector_value(0));
}

template<class T>
T *array<T>::get_vector_pointer() {
  php_assert (is_vector());
  return &(p->get_vector_value(0));
}

template<class T>
bool array<T>::is_equal_inner_pointer(const array &other) const noexcept {
  return p == other.p;
}

template<class T>
void swap(array<T> &lhs, array<T> &rhs) {
  lhs.swap(rhs);
}

template<class T>
const array<T> array_add(array<T> a1, const array<T> &a2) {
  return a1 += a2;
}

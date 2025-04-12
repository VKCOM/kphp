//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <type_traits>

#include "common/php-functions.h"
#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/stdlib/rpc/rpc-api.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"

void register_tl_storers_table_and_fetcher(const array<tl_storer_ptr>& gen$ht, tl_fetch_wrapper_ptr gen$t_ReqResult_fetch);

int32_t tl_parse_save_pos();

bool tl_parse_restore_pos(int32_t pos);

mixed tl_arr_get(const mixed& arr, const string& str_key, int64_t num_key, int64_t precomputed_hash = 0);

void store_magic_if_not_bare(uint32_t inner_magic);

void fetch_magic_if_not_bare(uint32_t inner_magic, const char* error_msg);

template<class T>
inline void fetch_raw_vector_T(array<T>& out __attribute__((unused)), int64_t n_elems __attribute__((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void fetch_raw_vector_T<double>(array<double>& out, int64_t n_elems) {
  fetch_raw_vector_double(out, n_elems);
}

template<class T>
inline void store_raw_vector_T(const array<T>& v __attribute__((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void store_raw_vector_T<double>(const array<double>& v) {
  store_raw_vector_double(v);
}

// Wrap into Optional that TL types which PhpType is:
//  1. int, double, string, bool
//  2. array<T>
// These types are not wrapped:
//  1. class_instance<T>
//  2. Optional<T>
//  3. mixed (long in TL scheme or mixed in the phpdoc) UPD: it will be wrapped after the int64_t transition is over

template<typename S>
struct need_Optional : vk::is_type_in_list<S, double, string, bool, int64_t> {};

template<typename S>
struct need_Optional<array<S>> : std::true_type {};

enum class FieldAccessType : uint8_t { read, write };

// C++14 if constexpr
template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline const typename SerializerT::PhpType& get_serialization_target_from_optional_field(const OptionalFieldT& v)
requires(need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::read)
{
  return v.val();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline typename SerializerT::PhpType& get_serialization_target_from_optional_field(OptionalFieldT& v)
requires(need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::write)
{
  return v.ref();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline OptionalFieldT& get_serialization_target_from_optional_field(OptionalFieldT& v)
requires(!need_Optional<typename SerializerT::PhpType>::value)
{
  return v;
}

struct t_Int {
  void store(const mixed& tl_object);
  int32_t fetch();

  using PhpType = int64_t;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);

  static int32_t prepare_int_for_storing(int64_t v);
};

struct t_Long {
  void store(const mixed& tl_object);
  mixed fetch();

  using PhpType = int64_t;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);
};

struct t_Double {
  void store(const mixed& tl_object);
  double fetch();

  using PhpType = double;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);
};

struct t_Float {
  void store(const mixed& tl_object);
  double fetch();

  using PhpType = double;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);
};

struct t_String {
  void store(const mixed& tl_object);
  string fetch();

  using PhpType = string;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);
};

struct t_Bool {
  void store(const mixed& tl_object);
  bool fetch();

  using PhpType = bool;
  void typed_store(const PhpType& v);
  void typed_fetch_to(PhpType& out);
};

struct t_True {
  void store(const mixed& v __attribute__((unused)));
  array<mixed> fetch();

  using PhpType = bool;
  void typed_store(const PhpType& v __attribute__((unused)));
  void typed_fetch_to(PhpType& out);
};

template<typename T, uint32_t inner_magic>
struct t_Vector {
  T elem_state;

  explicit t_Vector(T param_type)
      : elem_state(std::move(param_type)) {}

  void store(const mixed& v) {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) {
      cur_query.raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a = v.as_array();
    int64_t n = v.count();
    f$store_int(n);
    for (int64_t i = 0; i < n; ++i) {
      if (!a.isset(i)) {
        cur_query.raise_storing_error("Vector[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    //    CHECK_EXCEPTION(return array<mixed>());
    const auto size{f$fetch_int()};
    if (size < 0) {
      CurrentTlQuery::get().raise_fetching_error("Vector size is negative");
      return {};
    }

    array<mixed> res{array_size{size, true}};
    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      const mixed& elem{elem_state.fetch()};
      //      CHECK_EXCEPTION(return result);
      res.push_back(elem);
    }

    return res;
  }

  using PhpType = array<typename T::PhpType>;
  using PhpElemT = typename T::PhpType;

  void typed_store(const PhpType& v) {
    int64_t n = v.count();
    f$store_int(n);

    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector()) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < n; ++i) {
      if (!v.isset(i)) {
        CurrentTlQuery::get().raise_storing_error("Vector[%" PRIi64 "] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) {
    //    CHECK_EXCEPTION(return);
    const auto size{f$fetch_int()};
    if (size < 0) {
      CurrentTlQuery::get().raise_fetching_error("Vector size is negative");
      return;
    }

    out.reserve(size, true);
    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      fetch_raw_vector_T(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      PhpElemT elem;
      elem_state.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      //      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, uint32_t inner_magic>
struct t_Maybe {
  static_assert(!std::is_same_v<T, t_True>, "Usage (Maybe True) in TL is forbidden");

  T elem_state;

  explicit t_Maybe(T param_type)
      : elem_state(std::move(param_type)) {}

  // TODO: replace string{...} with constants
  void store(const mixed& v) {
    const string& name = f$strval(tl_arr_get(v, string{"_"}, 0, string_hash("_", 1)));
    if (name == string{"resultFalse"}) {
      f$store_int(TL_MAYBE_FALSE);
    } else if (name == string{"resultTrue"}) {
      f$store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.store(tl_arr_get(v, string{"result"}, 1, string_hash("result", 6)));
    } else {
      CurrentTlQuery::get().raise_storing_error("Constructor %s of type Maybe was not found in TL scheme", name.c_str());
    }
  }

  mixed fetch() {
    //    CHECK_EXCEPTION(return mixed());
    const auto magic{static_cast<uint32_t>(f$fetch_int())};
    switch (magic) {
    case TL_MAYBE_FALSE:
      return false;
    case TL_MAYBE_TRUE:
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
      return elem_state.fetch();
    default:
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
      return -1;
    }
  }

  static constexpr bool inner_needs_Optional = need_Optional<typename T::PhpType>::value;
  using PhpType = std::conditional_t<inner_needs_Optional, Optional<typename T::PhpType>, typename T::PhpType>;

  static bool has_maybe_value(const PhpType& v) {
    return !v.is_null();
  }

  void typed_store(const PhpType& v) {
    if (!has_maybe_value(v)) {
      f$store_int(TL_MAYBE_FALSE);
    } else {
      f$store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(get_serialization_target_from_optional_field<T, FieldAccessType::read>(v));
    }
  }

  void typed_fetch_to(PhpType& out) {
    //    CHECK_EXCEPTION(return);
    const auto magic{static_cast<uint32_t>(f$fetch_int())};
    switch (magic) {
    case TL_MAYBE_FALSE:
      // Wrapped into Optional: array<T>, int64_t, double, string, bool
      // Not wrapped:         : var, class_instance<T>, Optional<T>
      out = PhpType();
      break;
    case TL_MAYBE_TRUE:
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
      elem_state.typed_fetch_to(get_serialization_target_from_optional_field<T, FieldAccessType::write>(out));
      break;
    default:
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
      return;
    }
  }
};

template<typename KeyT, typename ValueT, uint32_t inner_value_magic>
struct tl_Dictionary_impl {
  ValueT value_state;

  explicit tl_Dictionary_impl(ValueT value_type)
      : value_state(std::move(value_type)) {}

  void store(const mixed& v) {
    if (!v.is_array()) {
      CurrentTlQuery::get().raise_storing_error("Expected array (dictionary), got something strange");
      return;
    }

    int64_t n = v.count();
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().store(it.get_key());
      store_magic_if_not_bare(inner_value_magic);
      value_state.store(it.get_value());
    }
  }

  array<mixed> fetch() {
    //    CHECK_EXCEPTION(return array<mixed>());
    const auto size{f$fetch_int()};
    if (size < 0) {
      CurrentTlQuery::get().raise_fetching_error("Dictionary size is negative");
      return {};
    }

    array<mixed> res{array_size{size, false}};
    for (int64_t i = 0; i < size; ++i) {
      const auto& key{KeyT().fetch()};
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");
      const mixed& value{value_state.fetch()};
      //      CHECK_EXCEPTION(return result);
      res.set_value(key, value);
    }
    return res;
  }

  using PhpType = array<typename ValueT::PhpType>;

  void typed_store(const PhpType& v) {
    int64_t n = v.count();
    f$store_int(n);

    for (auto it = v.begin(); it != v.end(); ++it) {
      if constexpr (std::is_same_v<KeyT, t_String>) {
        KeyT{}.typed_store(it.get_key().to_string());
      } else {
        KeyT{}.typed_store(it.get_key().to_int());
      }
      store_magic_if_not_bare(inner_value_magic);
      value_state.typed_store(it.get_value());
    }
  }

  void typed_fetch_to(PhpType& out) {
    //    CHECK_EXCEPTION(return);
    const auto size{f$fetch_int()};
    if (size < 0) {
      CurrentTlQuery::get().raise_fetching_error("Dictionary size is negative");
      return;
    }

    out.reserve(size, false);
    for (int64_t i = 0; i < size; ++i) {
      typename KeyT::PhpType key;
      KeyT().typed_fetch_to(key);
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");

      typename ValueT::PhpType elem;
      value_state.typed_fetch_to(elem);
      out.set_value(key, std::move(elem));
      //      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, uint32_t inner_magic>
using t_Dictionary = tl_Dictionary_impl<t_String, T, inner_magic>;

template<typename T, uint32_t inner_magic>
using t_IntKeyDictionary = tl_Dictionary_impl<t_Int, T, inner_magic>;

template<typename T, uint32_t inner_magic>
using t_LongKeyDictionary = tl_Dictionary_impl<t_Long, T, inner_magic>;

template<typename T, uint32_t inner_magic>
struct t_Tuple {
  T elem_state;
  int64_t size;

  t_Tuple(T param_type, int64_t size)
      : elem_state(std::move(param_type)),
        size(size) {}

  void store(const mixed& v) {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) {
      cur_query.raise_storing_error("Expected tuple, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a = v.as_array();
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        cur_query.raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    //    CHECK_EXCEPTION(return array<mixed>());
    array<mixed> res{array_size{size, true}};
    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      res.push_back(elem_state.fetch());
    }
    return res;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType& v) {
    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentTlQuery::get().raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) {
    //    CHECK_EXCEPTION(return);
    out.reserve(size, true);

    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      fetch_raw_vector_T(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      elem_state.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      //      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, uint32_t inner_magic>
struct tl_array {
  int64_t size;
  T cell;

  tl_array(int64_t size, T cell)
      : size(size),
        cell(std::move(cell)) {}

  void store(const mixed& v) {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) {
      cur_query.raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a = v.as_array();
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        cur_query.raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    array<mixed> result{array_size{size, true}};
    //    CHECK_EXCEPTION(return result);
    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      result.push_back(cell.fetch());
      //          CHECK_EXCEPTION(return result);
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType& v) {
    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentTlQuery::get().raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) {
    //    CHECK_EXCEPTION(return);
    out.reserve(size, true);

    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      cell.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      //      CHECK_EXCEPTION(return);
    }
  }
};

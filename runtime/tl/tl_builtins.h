// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/tl/constants/common.h"

#include "runtime/include.h"
#include "runtime/interface.h"
#include "runtime/rpc.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_tl_query.h"
#include "runtime/tl/tl_func_base.h"

#define tl_undefined_php_type std::nullptr_t

int tl_parse_save_pos();
bool tl_parse_restore_pos(int pos);

struct tl_exclamation_fetch_wrapper {
  std::unique_ptr<tl_func_base> fetcher;

  explicit tl_exclamation_fetch_wrapper(std::unique_ptr<tl_func_base> fetcher) :
    fetcher(std::move(fetcher)) {}

  tl_exclamation_fetch_wrapper() = default;
  tl_exclamation_fetch_wrapper(tl_exclamation_fetch_wrapper &&) = default;

  mixed fetch() {
    return fetcher->fetch();
  }

  using PhpType = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  void typed_fetch_to(PhpType &out) {
    php_assert(fetcher);
    out = fetcher->typed_fetch();
  }
};

using tl_storer_ptr = std::unique_ptr<tl_func_base>(*)(const mixed &);

inline mixed tl_arr_get(const mixed &arr, const string &str_key, int64_t num_key, int64_t precomputed_hash = 0) {
  if (!arr.is_array()) {
    CurrentProcessingQuery::get().raise_storing_error("Array expected, when trying to access field #%" PRIi64 " : %s", num_key, str_key.c_str());
    return {};
  }
  const mixed &num_v = arr.get_value(num_key);
  if (!num_v.is_null()) {
    return num_v;
  }
  const mixed &str_v = (precomputed_hash == 0 ? arr.get_value(str_key) : arr.get_value(str_key, precomputed_hash));
  if (!str_v.is_null()) {
    return str_v;
  }
  CurrentProcessingQuery::get().raise_storing_error("Field %s (#%" PRIi64 ") not found", str_key.c_str(), num_key);
  return {};
}

inline void store_magic_if_not_bare(unsigned int inner_magic) {
  if (inner_magic) {
    store_int(inner_magic);
  }
}

inline void fetch_magic_if_not_bare(unsigned int inner_magic, const char *error_msg) {
  if (inner_magic) {
    auto actual_magic = static_cast<unsigned int>(rpc_fetch_int());
    if (actual_magic != inner_magic) {
      CurrentProcessingQuery::get().raise_fetching_error("%s\nExpected 0x%08x, but fetched 0x%08x", error_msg, inner_magic, actual_magic);
    }
  }
}

template<class T>
inline void fetch_raw_vector_T(array<T> &out __attribute__ ((unused)), int64_t n_elems __attribute__ ((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void fetch_raw_vector_T<double>(array<double> &out, int64_t n_elems) {
  f$fetch_raw_vector_double(out, n_elems);
}

template<class T>
inline void store_raw_vector_T(const array<T> &v __attribute__ ((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void store_raw_vector_T<double>(const array<double> &v) {
  f$store_raw_vector_double(v);
}

// Wrap into Optional that TL types which PhpType is:
//  1. int, double, string, bool
//  2. array<T>
// These types are not wrapped:
//  1. class_instance<T>
//  2. Optional<T>
//  3. mixed (long in TL scheme or mixed in the phpdoc) UPD: it will be wrapped after the int64_t transition is over

template<typename S>
struct need_Optional : vk::is_type_in_list<S, double, string, bool, int64_t> {
};

template<typename S>
struct need_Optional<array<S>> : std::true_type {
};

enum class FieldAccessType {
  read,
  write
};

// C++14 if constexpr
template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline std::enable_if_t<need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::read,
                                      const typename SerializerT::PhpType &>
get_serialization_target_from_optional_field(const OptionalFieldT &v) {
  return v.val();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline std::enable_if_t<need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::write,
                                      typename SerializerT::PhpType &>
get_serialization_target_from_optional_field(OptionalFieldT &v) {
  return v.ref();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline std::enable_if_t<!need_Optional<typename SerializerT::PhpType>::value, OptionalFieldT &>
get_serialization_target_from_optional_field(OptionalFieldT &v) {
  return v;
}

struct t_Int {
  void store(const mixed &tl_object) {
    int32_t v32 = prepare_int_for_storing(f$intval(tl_object));
    store_int(v32);
  }

  int fetch() {
    CHECK_EXCEPTION(return 0);
    return rpc_fetch_int();
  }

  using PhpType = int64_t;

  void typed_store(const PhpType &v) {
    int32_t v32 = prepare_int_for_storing(v);
    store_int(v32);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = rpc_fetch_int();
  }

  static int32_t prepare_int_for_storing(int64_t v) {
    auto v32 = static_cast<int32_t>(v);
    if (unlikely(is_int32_overflow(v))) {
      if (fail_rpc_on_int32_overflow) {
        CurrentProcessingQuery::get().raise_storing_error("Got int32 overflow with value '%" PRIi64 "'. Serialization will fail.", v);
      } else {
        php_warning("Got int32 overflow on storing %s: the value '%" PRIi64 "' will be casted to '%d'. Serialization will succeed.",
                    CurrentProcessingQuery::get().get_current_tl_function_name().c_str(), v, v32);
      }
    }
    return v32;
  }
};

struct t_Long {
  void store(const mixed &tl_object) {
    store_long(tl_object);
  }

  mixed fetch() {
    CHECK_EXCEPTION(return mixed());
    return f$fetch_long();
  }

  using PhpType = int64_t;

  void typed_store(const PhpType &v) {
    f$store_long(v);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_long();
  }
};

struct t_Double {
  void store(const mixed &tl_object) {
    f$store_double(f$floatval(tl_object));
  }

  double fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_double();
  }

  using PhpType = double;

  void typed_store(const PhpType &v) {
    f$store_double(v);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_double();
  }
};

struct t_Float {
  void store(const mixed &tl_object) {
    f$store_float(f$floatval(tl_object));
  }

  double fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_float();
  }

  using PhpType = double;

  void typed_store(const PhpType &v) {
    f$store_float(v);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_float();
  }
};

struct t_String {
  void store(const mixed &tl_object) {
    f$store_string(f$strval(tl_object));
  }

  string fetch() {
    CHECK_EXCEPTION(return tl_str_);
    return f$fetch_string();
  }

  using PhpType = string;

  void typed_store(const PhpType &v) {
    f$store_string(f$strval(v));
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_string();
  }
};

struct t_Bool {
  void store(const mixed &tl_object) {
    store_int(tl_object.to_bool() ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }

  bool fetch() {
    CHECK_EXCEPTION(return false);
    auto magic = static_cast<unsigned int>(rpc_fetch_int());
    switch (magic) {
      case TL_BOOL_FALSE:
        return false;
      case TL_BOOL_TRUE:
        return true;
      default: {
        CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
        return -1;
      }
    }
  }

  using PhpType = bool;

  void typed_store(const PhpType &v) {
    store_int(v ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    auto magic = static_cast<unsigned int>(rpc_fetch_int());
    if (magic != TL_BOOL_TRUE && magic != TL_BOOL_FALSE) {
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
      return;
    }
    out = magic == TL_BOOL_TRUE;
  }
};

struct t_True {
  using PhpType = bool;

  void store(const mixed &__attribute__((unused))) {}

  array<mixed> fetch() {
    return {};
  }

  void typed_store(const PhpType &__attribute__((unused))) {}

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = true;
  }
};

template<typename T, unsigned int inner_magic>
struct t_Vector {
  T elem_state;

  explicit t_Vector(T param_type) :
    elem_state(std::move(param_type)) {}

  void store(const mixed &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<mixed> &a = v.as_array();
    int64_t n = v.count();
    f$store_int(n);
    for (int64_t i = 0; i < n; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Vector[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    CHECK_EXCEPTION(return array<mixed>());
    int n = rpc_fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Vector size is negative");
      return {};
    }
    array<mixed> result(array_size(std::min(n, 10000), 0, true));
    for (int i = 0; i < n; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      const mixed &cur_elem = elem_state.fetch();
      CHECK_EXCEPTION(return result);
      result.push_back(cur_elem);
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;
  using PhpElemT = typename T::PhpType;

  void typed_store(const PhpType &v) {
    int64_t n = v.count();
    f$store_int(n);

    if (std::is_same<T, t_Double>{} && inner_magic == 0 && v.is_vector()) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < n; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Vector[%" PRIi64 "] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    int n = rpc_fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Vector size is negative");
      return;
    }
    out.reserve(n, 0, true);

    if (std::is_same<T, t_Double>{} && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, n);
      return;
    }

    for (int i = 0; i < n; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      PhpElemT cur_elem;
      elem_state.typed_fetch_to(cur_elem);
      out.push_back(std::move(cur_elem));
      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, unsigned int inner_magic>
struct t_Maybe {
  static_assert(!std::is_same<T, t_True>::value, "usage (Maybe True) in TL is forbidden");

  T elem_state;

  explicit t_Maybe(T param_type) :
    elem_state(std::move(param_type)) {}

  void store(const mixed &v) {
    const string &name = f$strval(tl_arr_get(v, tl_str_underscore, 0, tl_str_underscore_hash));
    if (name == tl_str_resultFalse) {
      store_int(TL_MAYBE_FALSE);
    } else if (name == tl_str_resultTrue) {
      store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.store(tl_arr_get(v, tl_str_result, 1, tl_str_result_hash));
    } else {
      CurrentProcessingQuery::get().raise_storing_error("Constructor %s of type Maybe was not found in TL scheme", name.c_str());
      return;
    }
  }

  mixed fetch() {
    CHECK_EXCEPTION(return mixed());
    auto magic = static_cast<unsigned int>(rpc_fetch_int());
    switch (magic) {
      case TL_MAYBE_FALSE: {
        return false;
      }
      case TL_MAYBE_TRUE: {
        fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
        return elem_state.fetch();
      }
      default: {
        CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
        return -1;
      }
    }
  }

  static constexpr bool inner_needs_Optional = need_Optional<typename T::PhpType>::value;
  using PhpType = typename std::conditional<inner_needs_Optional, Optional<typename T::PhpType>, typename T::PhpType>::type;

  static bool has_maybe_value(const PhpType &v) {
    return !v.is_null();
  }

  void typed_store(const PhpType &v) {
    if (!has_maybe_value(v)) {
      store_int(TL_MAYBE_FALSE);
    } else {
      store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(get_serialization_target_from_optional_field<T, FieldAccessType::read>(v));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    auto magic = static_cast<unsigned int>(rpc_fetch_int());
    switch (magic) {
      case TL_MAYBE_FALSE: {
        // Wrapped into Optional: array<T>, int64_t, double, string, bool
        // Not wrapped:         : var, class_instance<T>, Optional<T>
        out = PhpType();
        break;
      }
      case TL_MAYBE_TRUE: {
        fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
        elem_state.typed_fetch_to(get_serialization_target_from_optional_field<T, FieldAccessType::write>(out));
        break;
      }
      default: {
        CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
        return;
      }
    }
  }
};

template<typename KeyT, typename ValueT, unsigned int inner_value_magic>
struct tl_Dictionary_impl {
  ValueT value_state;

  explicit tl_Dictionary_impl(ValueT value_type) :
    value_state(std::move(value_type)) {}

  void store(const mixed &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array (dictionary), got something strange");
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
    CHECK_EXCEPTION(return array<mixed>());
    array<mixed> result;
    int32_t n = rpc_fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Dictionary size is negative");
      return result;
    }
    for (int32_t i = 0; i < n; ++i) {
      const auto &key = KeyT().fetch();
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");
      const mixed &value = value_state.fetch();
      CHECK_EXCEPTION(return result);
      result.set_value(key, value);
    }
    return result;
  }

  using PhpType = array<typename ValueT::PhpType>;

  void typed_store(const PhpType &v) {
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

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    int32_t n = rpc_fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Dictionary size is negative");
      return;
    }
    for (int32_t i = 0; i < n; ++i) {
      typename KeyT::PhpType key;
      KeyT().typed_fetch_to(key);
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");
      typename ValueT::PhpType elem;
      value_state.typed_fetch_to(elem);
      out.set_value(key, std::move(elem));
      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, unsigned int inner_magic>
using t_Dictionary = tl_Dictionary_impl<t_String, T, inner_magic>;

template<typename T, unsigned int inner_magic>
using t_IntKeyDictionary = tl_Dictionary_impl<t_Int, T, inner_magic>;

template<typename T, unsigned int inner_magic>
using t_LongKeyDictionary = tl_Dictionary_impl<t_Long, T, inner_magic>;

template<typename T, unsigned int inner_magic>
struct t_Tuple {
  T elem_state;
  int64_t size;

  t_Tuple(T param_type, int64_t size) :
    elem_state(std::move(param_type)),
    size(size) {}

  void store(const mixed &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected tuple, got %s", v.get_type_c_str());
      return;
    }
    const array<mixed> &a = v.as_array();
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    CHECK_EXCEPTION(return array<mixed>());
    array<mixed> result(array_size(size, 0, true));
    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      result.push_back(elem_state.fetch());
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType &v) {
    if (std::is_same<T, t_Double>{} && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(int64_t{i}));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out.reserve(size, 0, true);

    if (std::is_same<T, t_Double>{} && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      elem_state.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      CHECK_EXCEPTION(return);
    }
  }
};

template<typename T, unsigned int inner_magic>
struct tl_array {
  int64_t size;
  T cell;

  tl_array(int64_t size, T cell) :
    size(size),
    cell(std::move(cell)) {}

  void store(const mixed &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<mixed> &a = v.as_array();
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.store(v.get_value(i));
    }
  }

  array<mixed> fetch() {
    array<mixed> result(array_size(size, 0, true));
    CHECK_EXCEPTION(return result);
    for (int64_t i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      result.push_back(cell.fetch());
      CHECK_EXCEPTION(return result);
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType &v) {
    if (std::is_same<T, t_Double>{} && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out.reserve(size, 0, true);

    if (std::is_same<T, t_Double>{} && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      cell.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      CHECK_EXCEPTION(return);
    }
  }
};

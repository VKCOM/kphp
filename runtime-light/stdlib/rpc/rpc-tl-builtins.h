//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "common/php-functions.h"
#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/fork/fork-state.h" // it's actually used by exception handling stuff
#include "runtime-light/stdlib/rpc/rpc-api.h"
#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/tl/tl-core.h"

inline void register_tl_storers_table_and_fetcher(const array<tl_storer_ptr>& gen$ht, tl_fetch_wrapper_ptr gen$t_ReqResult_fetch) noexcept {
  auto& rpc_mutable_image_state{RpcImageState::get_mutable()};
  rpc_mutable_image_state.tl_storers_ht = gen$ht;
  rpc_mutable_image_state.tl_storers_ht.set_reference_counter_to(ExtraRefCnt::for_global_const);
  rpc_mutable_image_state.tl_fetch_wrapper = gen$t_ReqResult_fetch;
}

inline int32_t tl_parse_save_pos() noexcept {
  return static_cast<int32_t>(RpcClientInstanceState::get().rpc_buffer.pos());
}

inline bool tl_parse_restore_pos(int32_t pos) noexcept {
  auto& rpc_buf{RpcClientInstanceState::get().rpc_buffer};
  if (pos < 0 || pos > rpc_buf.pos()) [[unlikely]] {
    return false;
  }
  rpc_buf.reset(pos);
  return true;
}

mixed tl_arr_get(const mixed& arr, const string& str_key, int64_t num_key, int64_t precomputed_hash = 0) noexcept;

inline void store_magic_if_not_bare(uint32_t inner_magic) noexcept {
  if (static_cast<bool>(inner_magic)) [[likely]] {
    f$store_int(inner_magic);
  }
}

inline void fetch_magic_if_not_bare(uint32_t inner_magic, const char* error_msg) noexcept {
  CHECK_EXCEPTION(return);
  if (static_cast<bool>(inner_magic)) [[likely]] {
    const auto actual_magic{static_cast<uint32_t>(TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int()))};
    if (actual_magic != inner_magic) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("%s\nExpected 0x%08x, but fetched 0x%08x", error_msg, inner_magic, actual_magic);
    }
  }
}

template<class T>
inline void fetch_raw_vector_T(array<T>& /*out*/, int64_t /*n_elems*/) noexcept {
  php_assert(0 && "never called in runtime");
}

template<>
inline void fetch_raw_vector_T<double>(array<double>& out, int64_t n_elems) noexcept {
  f$fetch_raw_vector_double(out, n_elems);
}

template<class T>
inline void store_raw_vector_T(const array<T>& /*v*/) noexcept {
  php_assert(0 && "never called in runtime");
}

template<>
inline void store_raw_vector_T<double>(const array<double>& v) noexcept {
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
struct need_Optional : vk::is_type_in_list<S, double, string, bool, int64_t> {};

template<typename S>
struct need_Optional<array<S>> : std::true_type {};

enum class FieldAccessType : uint8_t { read, write };

// C++14 if constexpr
template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline const typename SerializerT::PhpType& get_serialization_target_from_optional_field(const OptionalFieldT& v) noexcept
requires(need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::read)
{
  return v.val();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline typename SerializerT::PhpType& get_serialization_target_from_optional_field(OptionalFieldT& v) noexcept
requires(need_Optional<typename SerializerT::PhpType>::value && ac == FieldAccessType::write)
{
  return v.ref();
}

template<typename SerializerT, FieldAccessType ac, typename OptionalFieldT>
inline OptionalFieldT& get_serialization_target_from_optional_field(OptionalFieldT& v) noexcept
requires(!need_Optional<typename SerializerT::PhpType>::value)
{
  return v;
}

struct t_Int {
  void store(const mixed& tl_object) noexcept {
    int32_t v32{prepare_int_for_storing(f$intval(tl_object))};
    f$store_int(v32);
  }
  int32_t fetch() noexcept {
    CHECK_EXCEPTION(return {});
    return f$fetch_int();
  }

  using PhpType = int64_t;
  void typed_store(const t_Int::PhpType& v) noexcept {
    int32_t v32{prepare_int_for_storing(v)};
    f$store_int(v32);
  }
  void typed_fetch_to(t_Int::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = f$fetch_int();
  }
  static int32_t prepare_int_for_storing(int64_t v) noexcept;
};

struct t_Long {
  void store(const mixed& tl_object) noexcept {
    int64_t v64{f$intval(tl_object)};
    f$store_long(v64);
  }
  mixed fetch() noexcept {
    CHECK_EXCEPTION(return {});
    return f$fetch_long();
  }

  using PhpType = int64_t;
  void typed_store(const t_Long::PhpType& v) noexcept {
    f$store_long(v);
  }
  void typed_fetch_to(t_Long::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = f$fetch_long();
  }
};

struct t_Double {
  void store(const mixed& tl_object) noexcept {
    f$store_double(f$floatval(tl_object));
  }
  double fetch() noexcept {
    CHECK_EXCEPTION(return {});
    return f$fetch_double();
  }

  using PhpType = double;
  void typed_store(const t_Double::PhpType& v) noexcept {
    f$store_double(v);
  }
  void typed_fetch_to(t_Double::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = f$fetch_double();
  }
};

struct t_Float {
  void store(const mixed& tl_object) noexcept {
    f$store_float(f$floatval(tl_object));
  }
  double fetch() noexcept {
    CHECK_EXCEPTION(return {});
    return f$fetch_float();
  }

  using PhpType = double;
  void typed_store(const t_Float::PhpType& v) noexcept {
    f$store_float(v);
  }
  void typed_fetch_to(t_Float::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = f$fetch_float();
  }
};

struct t_String {
  void store(const mixed& tl_object) noexcept {
    f$store_string(f$strval(tl_object));
  }
  string fetch() noexcept {
    CHECK_EXCEPTION(return {});
    return f$fetch_string();
  }

  using PhpType = string;
  void typed_store(const t_String::PhpType& v) noexcept {
    f$store_string(f$strval(v));
  }
  void typed_fetch_to(t_String::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = f$fetch_string();
  }
};

struct t_Bool {
  void store(const mixed& tl_object) noexcept {
    f$store_int(tl_object.to_bool() ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }
  bool fetch() noexcept {
    CHECK_EXCEPTION(return {});
    const auto magic{static_cast<uint32_t>(TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int()))};
    switch (magic) {
    case TL_BOOL_FALSE:
      return false;
    case TL_BOOL_TRUE:
      return true;
    default: {
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
      return false;
    }
    }
  }

  using PhpType = bool;
  void typed_store(const t_Bool::PhpType& v) noexcept {
    f$store_int(v ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }
  void typed_fetch_to(t_Bool::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    const auto magic{static_cast<uint32_t>(TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int()))};
    if (magic != TL_BOOL_TRUE && magic != TL_BOOL_FALSE) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
      return;
    }
    out = magic == TL_BOOL_TRUE;
  }
};

struct t_True {
  void store(const mixed& /*v*/) noexcept {}
  array<mixed> fetch() noexcept {
    return {};
  }

  using PhpType = bool;
  void typed_store(const t_True::PhpType& /*v*/) noexcept {}
  void typed_fetch_to(t_True::PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out = true;
  }
};

template<typename T, uint32_t inner_magic>
struct t_Vector {
  T elem_state;

  explicit t_Vector(T param_type) noexcept
      : elem_state(std::move(param_type)) {}

  void store(const mixed& v) noexcept {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) [[unlikely]] {
      cur_query.raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a{v.as_array()};
    int64_t n{v.count()};
    f$store_int(n);
    for (int64_t i = 0; i < n; ++i) {
      if (!a.isset(i)) [[unlikely]] {
        cur_query.raise_storing_error("Vector[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() noexcept {
    CHECK_EXCEPTION(return {});
    const auto size{TRY_CALL(decltype(f$fetch_int()), array<mixed>, f$fetch_int())};
    if (size < 0) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("Vector size is negative");
      return {};
    }

    array<mixed> res{array_size{size, true}};
    for (int64_t i = 0; i < size; ++i) {
      TRY_CALL_VOID(array<mixed>, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector"));
      const mixed& elem{elem_state.fetch()};
      res.push_back(elem);
    }

    return res;
  }

  using PhpType = array<typename T::PhpType>;
  using PhpElemT = typename T::PhpType;

  void typed_store(const PhpType& v) noexcept {
    int64_t n{v.count()};
    f$store_int(n);

    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector()) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < n; ++i) {
      if (!v.isset(i)) [[unlikely]] {
        CurrentTlQuery::get().raise_storing_error("Vector[%" PRIi64 "] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    const auto size{TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int())};
    if (size < 0) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("Vector size is negative");
      return;
    }

    out.reserve(size, true);
    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      fetch_raw_vector_T(out, size);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      TRY_CALL_VOID(void, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector"));
      PhpElemT elem;
      elem_state.typed_fetch_to(elem);
      out.push_back(std::move(elem));
    }
  }
};

template<typename T, uint32_t inner_magic>
struct t_Maybe {
  static_assert(!std::is_same_v<T, t_True>, "Usage (Maybe True) in TL is forbidden");

  T elem_state;

  explicit t_Maybe(T param_type) noexcept
      : elem_state(std::move(param_type)) {}

  void store(const mixed& v) noexcept {
    const string& name{f$strval(tl_arr_get(v, string{"_"}, 0, string_hash("_", 1)))};
    const std::string_view name_view{name.c_str(), name.size()};
    if (name_view == "resultFalse") {
      f$store_int(TL_MAYBE_FALSE);
    } else if (name_view == "resultTrue") {
      f$store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.store(tl_arr_get(v, string{"result"}, 1, string_hash("result", 6)));
    } else {
      CurrentTlQuery::get().raise_storing_error("Constructor %s of type Maybe was not found in TL scheme", name.c_str());
    }
  }

  mixed fetch() noexcept {
    CHECK_EXCEPTION(return {});
    const auto magic{static_cast<uint32_t>(TRY_CALL(decltype(f$fetch_int()), mixed, f$fetch_int()))};
    switch (magic) {
    case TL_MAYBE_FALSE:
      return false;
    case TL_MAYBE_TRUE:
      TRY_CALL_VOID(mixed, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe"));
      return elem_state.fetch();
    default:
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
      return -1;
    }
  }

  static constexpr bool inner_needs_Optional = need_Optional<typename T::PhpType>::value;
  using PhpType = std::conditional_t<inner_needs_Optional, Optional<typename T::PhpType>, typename T::PhpType>;

  static bool has_maybe_value(const PhpType& v) noexcept {
    return !v.is_null();
  }

  void typed_store(const PhpType& v) noexcept {
    if (!has_maybe_value(v)) {
      f$store_int(TL_MAYBE_FALSE);
    } else {
      f$store_int(TL_MAYBE_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(get_serialization_target_from_optional_field<T, FieldAccessType::read>(v));
    }
  }

  void typed_fetch_to(PhpType& out) {
    CHECK_EXCEPTION(return);
    const auto magic{static_cast<uint32_t>(TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int()))};
    switch (magic) {
    case TL_MAYBE_FALSE:
      // Wrapped into Optional: array<T>, int64_t, double, string, bool
      // Not wrapped:         : var, class_instance<T>, Optional<T>
      out = PhpType();
      break;
    case TL_MAYBE_TRUE:
      TRY_CALL_VOID(void, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe"));
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

  explicit tl_Dictionary_impl(ValueT value_type) noexcept
      : value_state(std::move(value_type)) {}

  void store(const mixed& v) noexcept {
    if (!v.is_array()) [[unlikely]] {
      CurrentTlQuery::get().raise_storing_error("Expected array (dictionary), got something strange");
      return;
    }

    int64_t n{v.count()};
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().store(it.get_key());
      store_magic_if_not_bare(inner_value_magic);
      value_state.store(it.get_value());
    }
  }

  array<mixed> fetch() noexcept {
    CHECK_EXCEPTION(return {});
    const auto size{TRY_CALL(decltype(f$fetch_int()), array<mixed>, f$fetch_int())};
    if (size < 0) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("Dictionary size is negative");
      return {};
    }

    array<mixed> res{array_size{size, false}};
    for (int64_t i = 0; i < size; ++i) {
      const auto& key{KeyT().fetch()};
      TRY_CALL_VOID(array<mixed>, fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary"));
      const mixed& value{value_state.fetch()};
      res.set_value(key, value);
    }
    return res;
  }

  using PhpType = array<typename ValueT::PhpType>;

  void typed_store(const PhpType& v) noexcept {
    int64_t n{v.count()};
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

  void typed_fetch_to(PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    const auto size{TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int())};
    if (size < 0) [[unlikely]] {
      CurrentTlQuery::get().raise_fetching_error("Dictionary size is negative");
      return;
    }

    out.reserve(size, false);
    for (int64_t i = 0; i < size; ++i) {
      typename KeyT::PhpType key;
      KeyT().typed_fetch_to(key);
      TRY_CALL_VOID(void, fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary"));

      typename ValueT::PhpType elem;
      value_state.typed_fetch_to(elem);
      out.set_value(key, std::move(elem));
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

  t_Tuple(T param_type, int64_t size) noexcept
      : elem_state(std::move(param_type)),
        size(size) {}

  void store(const mixed& v) noexcept {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) [[unlikely]] {
      cur_query.raise_storing_error("Expected tuple, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a{v.as_array()};
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) [[unlikely]] {
        cur_query.raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<mixed> fetch() noexcept {
    CHECK_EXCEPTION(return {});
    array<mixed> res{array_size{size, true}};
    for (int64_t i = 0; i < size; ++i) {
      TRY_CALL_VOID(array<mixed>, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple"));
      res.push_back(elem_state.fetch());
    }
    return res;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType& v) noexcept {
    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) [[unlikely]] {
        CurrentTlQuery::get().raise_storing_error("Tuple[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out.reserve(size, true);
    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      TRY_CALL_VOID(void, fetch_raw_vector_T(out, size));
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      TRY_CALL_VOID(void, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple"));
      elem_state.typed_fetch_to(elem);
      out.push_back(std::move(elem));
    }
  }
};

template<typename T, uint32_t inner_magic>
struct tl_array {
  int64_t size;
  T cell;

  tl_array(int64_t size, T cell) noexcept
      : size(size),
        cell(std::move(cell)) {}

  void store(const mixed& v) noexcept {
    const auto& cur_query{CurrentTlQuery::get()};

    if (!v.is_array()) [[unlikely]] {
      cur_query.raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }

    const array<mixed>& a{v.as_array()};
    for (int64_t i = 0; i < size; ++i) {
      if (!a.isset(i)) [[unlikely]] {
        cur_query.raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.store(v.get_value(i));
    }
  }

  array<mixed> fetch() noexcept {
    CHECK_EXCEPTION(return {});
    array<mixed> result{array_size{size, true}};
    for (int64_t i = 0; i < size; ++i) {
      TRY_CALL_VOID(array<mixed>, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array"));
      result.push_back(cell.fetch());
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType& v) noexcept {
    if (std::is_same_v<T, t_Double> && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      if (!v.isset(i)) [[unlikely]] {
        CurrentTlQuery::get().raise_storing_error("Array[%ld] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType& out) noexcept {
    CHECK_EXCEPTION(return);
    out.reserve(size, true);
    if (std::is_same_v<T, t_Double> && inner_magic == 0) {
      TRY_CALL_VOID(void, fetch_raw_vector_T<typename T::PhpType>(out, size));
      return;
    }

    for (int64_t i = 0; i < size; ++i) {
      typename T::PhpType elem;
      TRY_CALL_VOID(void, fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array"));
      cell.typed_fetch_to(elem);
      out.push_back(std::move(elem));
    }
  }
};

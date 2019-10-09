#pragma once

#include <memory>

#include "common/type_traits/constexpr_if.h"

#include "runtime/include.h"
#include "runtime/interface.h"
#include "runtime/rpc.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_query.h"

#define TL_INT 0xa8509bda
#define TL_LONG 0x22076cba
#define TL_DOUBLE 0x2210c154
#define TL_STRING 0xb5286e24
#define TL_BOOL_FALSE 0xbc799737
#define TL_BOOL_TRUE 0x997275b5
#define TL_VECTOR 0x1cb5c415
#define TL_TUPLE 0x9770768a
#define TL_RESULT_FALSE 0x27930a7b
#define TL_RESULT_TRUE 0x3f9c8ef8

#define tl_undefined_php_type nullptr_t

const string tl_str_("");
const string tl_str_underscore("_");
const string tl_str_resultFalse("resultFalse");
const string tl_str_resultTrue("resultTrue");
const string tl_str_result("result");

const int tl_str_underscore_hash = string_hash("_", 1);
const int tl_str_result_hash = string_hash("result", 6);

int tl_parse_save_pos();
bool tl_parse_restore_pos(int pos);

struct tl_func_base : ManagedThroughDlAllocator {
  virtual var fetch() = 0;
  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    // все функции, вызывающиеся типизированно, кодогенерированно переопределяют этот метод
    // а функции, типизированно не вызывающиеся, никогда не будут вызваны
    // (не стали делать её чистой виртуальной, чтобы для не типизированных не переопределять на "return {};")
    php_critical_error("This function should be never called, only to be overridden");
    return {};
  }

  // каждая плюсовая tl-функция ещё обладает
  // static std::unique_ptr<tl_func_base> store(const var &tl_object);
  // static std::unique_ptr<tl_func_base> typed_store(const C$VK$TL$Functions$thisfunction *tl_object);
  // они не виртуальные, т.к. static, но кодогенерятся в каждой
  // каждая из них создаёт инстанс себя (fetcher), на котором вызываются fetch()/typed_fetch(), когда ответ получен

  virtual ~tl_func_base() = default;
};

struct tl_exclamation_fetch_wrapper {
  std::unique_ptr<tl_func_base> fetcher;

  explicit tl_exclamation_fetch_wrapper(std::unique_ptr<tl_func_base> fetcher) :
    fetcher(std::move(fetcher)) {}

  tl_exclamation_fetch_wrapper() = default;
  tl_exclamation_fetch_wrapper(tl_exclamation_fetch_wrapper &&) = default;

  var fetch() {
    return fetcher->fetch();
  }

  using PhpType = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  void typed_fetch_to(PhpType &out) {
    php_assert(fetcher);
    out = fetcher->typed_fetch();
  }
};

using tl_storer_ptr = std::unique_ptr<tl_func_base>(*)(const var &);

inline var tl_arr_get(const var &arr, const string &str_key, int num_key, int precomputed_hash = 0) {
  if (!arr.is_array()) {
    CurrentProcessingQuery::get().raise_storing_error("Array expected, when trying to access field #%d : %s", num_key, str_key.c_str());
    return var();
  }
  const var &num_v = arr.get_value(num_key);
  if (!num_v.is_null()) {
    return num_v;
  }
  const var &str_v = (precomputed_hash == 0 ? arr.get_value(str_key) : arr.get_value(str_key, precomputed_hash));
  if (!str_v.is_null()) {
    return str_v;
  }
  CurrentProcessingQuery::get().raise_storing_error("Field %s (#%d) not found", str_key.c_str(), num_key);
  return var();
}

inline void store_magic_if_not_bare(unsigned int inner_magic) {
  if (inner_magic) {
    f$store_int(inner_magic);
  }
}

inline void fetch_magic_if_not_bare(unsigned int inner_magic, const char *error_msg) {
  if (inner_magic) {
    auto actual_magic = static_cast<unsigned int>(f$fetch_int());
    if (actual_magic != inner_magic) {
      CurrentProcessingQuery::get().raise_fetching_error("%s\nExpected 0x%08x, but fetched 0x%08x", error_msg, inner_magic, actual_magic);
    }
  }
}

template<class T>
inline void fetch_raw_vector_T(array<T> &out __attribute__ ((unused)), int n_elems __attribute__ ((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void fetch_raw_vector_T<int>(array<int> &out, int n_elems) {
  f$fetch_raw_vector_int(out, n_elems);
}

template<>
inline void fetch_raw_vector_T<double>(array<double> &out, int n_elems) {
  f$fetch_raw_vector_double(out, n_elems);
}

template<class T>
inline void store_raw_vector_T(const array<T> &v __attribute__ ((unused))) {
  php_assert(0 && "never called in runtime");
}

template<>
inline void store_raw_vector_T<int>(const array<int> &v) {
  f$store_raw_vector_int(v);
}

template<>
inline void store_raw_vector_T<double>(const array<double> &v) {
  f$store_raw_vector_double(v);
}

struct t_Int {
  void store(const var &tl_object) {
    f$store_int(f$intval(tl_object));
  }

  int fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_int();
  }

  using PhpType = int;

  void typed_store(const PhpType &v) {
    f$store_int(f$safe_intval(v));
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_int();
  }
};

struct t_Long {
  void store(const var &tl_object) {
    f$store_long(tl_object);
  }

  var fetch() {
    CHECK_EXCEPTION(return var());
    return f$fetch_long();
  }

  using PhpType = var;

  void typed_store(const PhpType &v) {
    f$store_long(v);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_long();
  }
};

struct t_Double {
  void store(const var &tl_object) {
    f$store_double(f$floatval(tl_object));
  }

  double fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_double();
  }

  using PhpType = double;

  void typed_store(const PhpType &v) {
    f$store_double(f$floatval(v));
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out = f$fetch_double();
  }
};

struct t_String {
  void store(const var &tl_object) {
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
  void store(const var &tl_object) {
    f$store_int(tl_object.to_bool() ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }

  bool fetch() {
    CHECK_EXCEPTION(return false);
    auto magic = static_cast<unsigned int>(f$fetch_int());
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
    f$store_int(v ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    auto magic = static_cast<unsigned int>(f$fetch_int());
    if (magic != TL_BOOL_TRUE && magic != TL_BOOL_FALSE) {
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
      return;
    }
    out = magic == TL_BOOL_TRUE;
  }
};

struct t_True {
  using PhpType = bool;

  void store(const var &__attribute__((unused))) {}

  array<var> fetch() {
    return array<var>();
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

  void store(const var &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    int n = v.count();
    f$store_int(n);
    for (int i = 0; i < n; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Vector[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    int n = f$fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Vector size is negative");
      return array<var>();
    }
    array<var> result(array_size(std::min(n, 10000), 0, true));
    for (int i = 0; i < n; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      const var &cur_elem = elem_state.fetch();
      CHECK_EXCEPTION(return result);
      result.push_back(cur_elem);
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;
  using PhpElemT = typename T::PhpType;

  void typed_store(const PhpType &v) {
    int n = v.count();
    f$store_int(n);

    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0 && v.is_vector()) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int i = 0; i < n; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Vector[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    int n = f$fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Vector size is negative");
      return;
    }
    out.reserve(n, 0, true);

    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0) {
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
  T elem_state;

  explicit t_Maybe(T param_type) :
    elem_state(std::move(param_type)) {}

  void store(const var &v) {
    const string &name = f$strval(tl_arr_get(v, tl_str_underscore, 0, tl_str_underscore_hash));
    if (name == tl_str_resultFalse) {
      f$store_int(TL_RESULT_FALSE);
    } else if (name == tl_str_resultTrue) {
      f$store_int(TL_RESULT_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.store(tl_arr_get(v, tl_str_result, 1, tl_str_result_hash));
    } else {
      CurrentProcessingQuery::get().raise_storing_error("Constructor %s of type Maybe was not found in TL scheme", name.c_str());
      return;
    }
  }

  var fetch() {
    CHECK_EXCEPTION(return var());
    auto magic = static_cast<unsigned int>(f$fetch_int());
    switch (magic) {
      case TL_RESULT_FALSE: {
        return false;
      }
      case TL_RESULT_TRUE: {
        fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
        return elem_state.fetch();
      }
      default: {
        CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
        return -1;
      }
    }
  }

  template<typename>
  struct is_array : std::false_type {
  };

  template<typename S>
  struct is_array<array<S>> : std::true_type {
  };

  template<typename S>
  struct need_OrFalse {
    static constexpr bool value = std::is_same<S, int>::value || std::is_same<S, double>::value || std::is_same<S, string>::value
                                  || is_array<S>::value;
  };

  static constexpr bool is_OrFalse = need_OrFalse<typename T::PhpType>::value;
  // На текущий момент OrFalse не нужен если T::PhpType -- class_instance, bool, OrFalse или var (long (mixed в php-doc))
  using PhpType = typename std::conditional<is_OrFalse, OrFalse<typename T::PhpType>, typename T::PhpType>::type;

  // C++11 if constexpr
  template<typename S>
  typename std::enable_if<need_OrFalse<typename S::PhpType>::value, const typename S::PhpType &>::type
  get_store_target(const PhpType &v) {
    return v.val();
  }

  template<typename S>
  typename std::enable_if<!need_OrFalse<typename S::PhpType>::value, const typename S::PhpType &>::type
  get_store_target(const PhpType &v) {
    return v;
  }

  template<typename S>
  typename std::enable_if<need_OrFalse<typename S::PhpType>::value, typename S::PhpType &>::type
  get_fetch_target(PhpType &v) {
    return v.ref();
  }

  template<typename S>
  typename std::enable_if<!need_OrFalse<typename S::PhpType>::value, typename S::PhpType &>::type
  get_fetch_target(PhpType &v) {
    return v;
  }

  void typed_store(const PhpType &v) {
    if (!f$boolval(v)) {
      f$store_int(TL_RESULT_FALSE);
    } else {
      f$store_int(TL_RESULT_TRUE);
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(get_store_target<T>(v));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    auto magic = static_cast<unsigned int>(f$fetch_int());
    switch (magic) {
      case TL_RESULT_FALSE: {
        out = false;
        break;
      }
      case TL_RESULT_TRUE: {
        fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
        elem_state.typed_fetch_to(get_fetch_target<T>(out));
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

  void store(const var &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array (dictionary), got something strange");
      return;
    }
    int n = v.count();
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().store(it.get_key());
      store_magic_if_not_bare(inner_value_magic);
      value_state.store(it.get_value());
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    array<var> result;
    int n = f$fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Dictionary size is negative");
      return result;
    }
    for (int i = 0; i < n; ++i) {
      const auto &key = KeyT().fetch();
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");
      const var &value = value_state.fetch();
      CHECK_EXCEPTION(return result);
      result.set_value(key, value);
    }
    return result;
  }

  using PhpType = array<typename ValueT::PhpType>;

  void typed_store(const PhpType &v) {
    int n = v.count();
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().typed_store(vk::constexpr_if(std::integral_constant<bool, std::is_same<KeyT, t_String>::value>{},
                                          [&] { return it.get_key().as_string(); },
                                          [&] { return vk::constexpr_if(std::integral_constant<bool, std::is_same<KeyT, t_Int>::value>{},
                                                               [&]() { return it.get_key().as_int(); },
                                                               [&]() { return it.get_key(); }); }
                                                               ));
      store_magic_if_not_bare(inner_value_magic);
      value_state.typed_store(it.get_value());
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    int n = f$fetch_int();
    if (n < 0) {
      CurrentProcessingQuery::get().raise_fetching_error("Dictionary size is negative");
      return;
    }
    for (int i = 0; i < n; ++i) {
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
  int size;

  t_Tuple(T param_type, int size) :
    elem_state(std::move(param_type)),
    size(size) {}

  void store(const var &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected tuple, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    for (int i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Tuple[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    array<var> result(array_size(size, 0, true));
    for (int i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      result.push_back(elem_state.fetch());
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType &v) {
    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Tuple[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      elem_state.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out.reserve(size, 0, true);

    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, size);
      return;
    }

    for (int i = 0; i < size; ++i) {
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
  int size;
  T cell;

  tl_array(int size, T cell) :
    size(size),
    cell(std::move(cell)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      CurrentProcessingQuery::get().raise_storing_error("Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    for (int i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Array[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    array<var> result(array_size(size, 0, true));
    CHECK_EXCEPTION(return result);
    for (int i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      result.push_back(cell.fetch());
      CHECK_EXCEPTION(return result);
    }
    return result;
  }

  using PhpType = array<typename T::PhpType>;

  void typed_store(const PhpType &v) {
    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0 && v.is_vector() && v.count() == size) {
      store_raw_vector_T<typename T::PhpType>(v);
      return;
    }

    for (int i = 0; i < size; ++i) {
      if (!v.isset(i)) {
        CurrentProcessingQuery::get().raise_storing_error("Array[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      cell.typed_store(v.get_value(i));
    }
  }

  void typed_fetch_to(PhpType &out) {
    CHECK_EXCEPTION(return);
    out.reserve(size, 0, true);

    if ((std::is_same<T, t_Int>{} || std::is_same<T, t_Double>{}) && inner_magic == 0) {
      fetch_raw_vector_T<typename T::PhpType>(out, size);
      return;
    }

    for (int i = 0; i < size; ++i) {
      typename T::PhpType elem;
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of tl array");
      cell.typed_fetch_to(elem);
      out.push_back(std::move(elem));
      CHECK_EXCEPTION(return);
    }
  }
};

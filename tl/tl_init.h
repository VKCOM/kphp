#pragma once

#include <memory>

#include "runtime/include.h"
#include "runtime/interface.h"
#include "auto/TL/constants.h"
#include "runtime/rpc.h"

const string tl_str_("");
const string tl_str_underscore("_");
const string tl_str_boolFalse("boolFalse");
const string tl_str_boolTrue("boolTrue");
const string tl_str_resultFalse("resultFalse");
const string tl_str_resultTrue("resultTrue");
const string tl_str_result("result");

const int tl_str_underscore_hash = string_hash("_", 1);
const int tl_str_result_hash     = string_hash("result", 6);

inline std::string dump_tl_array(const var &v) {
  do_var_dump(v, 0);
  string to_print = f$ob_get_contents();
  f$ob_clean();
  return to_print.c_str();
}

extern bool new_tl_mode_error_flag;

#define tl_storing_error(tl_object, format, ...) \
  if (!new_tl_mode_error_flag) { \
    new_tl_mode_error_flag = true; \
    php_warning("Storing error:\n" format "\nIn %s during serialization of the following TL object:\n%s", ##__VA_ARGS__, new_tl_current_function_name, dump_tl_array(tl_object).c_str()); \
  };

#define tl_fetching_error(exception_msg, format, ...) \
  if (CurException.is_null()) { \
    php_warning("Fetching error:\n" format, ##__VA_ARGS__); \
    THROW_EXCEPTION(f$Exception$$__construct(string(__FILE__), __LINE__, string(exception_msg), -1)); \
  };

int tl_parse_save_pos();
bool tl_parse_restore_pos(int pos);

struct tl_func_base {
  virtual var fetch() = 0;

  virtual const char *get_name() const = 0;
  virtual size_t get_sizeof() const = 0;

  virtual ~tl_func_base() = default;


  void *operator new(size_t sz) {
    return dl::allocate(sz);
  }

  void *operator new[](size_t sz) {
    return dl::allocate(sz);
  }

  void operator delete(void *ptr) {
    dl::deallocate(ptr, static_cast<dl::size_type>(static_cast<tl_func_base *>(ptr)->get_sizeof()));
  }

  void operator delete[](void *ptr) {
    dl::deallocate(ptr, static_cast<dl::size_type>(static_cast<tl_func_base *>(ptr)->get_sizeof()));
  }
};

struct tl_exclamation_fetch_wrapper {
  std::unique_ptr<tl_func_base> fetcher;

  tl_exclamation_fetch_wrapper() = default;

  tl_exclamation_fetch_wrapper(tl_exclamation_fetch_wrapper &&other) noexcept :
    fetcher(std::move(other.fetcher)) {}

  var fetch() {
    return fetcher->fetch();
  }
};

using tl_storer_ptr = std::unique_ptr<tl_func_base>(*)(const var &);

extern array<tl_storer_ptr> tl_storers_ht;

inline var tl_arr_get(const var &arr, const string &str_key, int num_key, int precomputed_hash = 0) {
  if (!arr.is_array()) {
    tl_storing_error(arr, "Array expected, when trying to access field #%d : %s", num_key, str_key.c_str());
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
  tl_storing_error(arr, "Field %s (#%d) not found while storing %s", str_key.c_str(), num_key, new_tl_current_function_name);
  return var();
}

template<bool is_bare>
struct tl_Int_impl {
  void store(const var &tl_object) {
    if (!is_bare) {
      f$store_int(TL_INT);
    }
    f$store_int(f$safe_intval(tl_object));
  }

  int fetch() {
    CHECK_EXCEPTION(return 0);
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_INT) {
        tl_fetching_error("Incorrect magic of type Int", "Incorrect magic of type Int: %08x", magic);
        return 0;
      }
    }
    return f$fetch_int();
  }
};

using t_Int = tl_Int_impl<false>;
using t_Int_$ = tl_Int_impl<true>;

template<bool is_bare>
struct tl_Long_impl {
  void store(const var &tl_object) {
    if (!is_bare) {
      f$store_int(TL_LONG);
    }
    f$store_long(tl_object);
  }

  var fetch() {
    CHECK_EXCEPTION(return var());
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_LONG) {
        tl_fetching_error("Incorrect magic of type Long", "Incorrect magic of type Long: %08x", magic);
        return var();
      }
    }
    return f$fetch_long();
  }
};

using t_Long = tl_Long_impl<false>;
using t_Long_$ = tl_Long_impl<true>;

template<bool is_bare>
struct tl_Double_impl {
  void store(const var &tl_object) {
    if (!is_bare) {
      f$store_int(TL_DOUBLE);
    }
    f$store_double(f$floatval(tl_object));
  }

  double fetch() {
    CHECK_EXCEPTION(return 0);
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_DOUBLE) {
        tl_fetching_error("Incorrect magic of type Double", "Incorrect magic of type Double: %08x", magic);
        return 0;
      }
    }
    return f$fetch_double();
  }
};

using t_Double = tl_Double_impl<false>;
using t_Double_$ = tl_Double_impl<true>;

template<bool is_bare>
struct tl_String_impl {
  void store(const var &tl_object) {
    if (!is_bare) {
      f$store_int(TL_STRING);
    }
    f$store_string(f$strval(tl_object));
  }

  string fetch() {
    CHECK_EXCEPTION(return tl_str_);
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_STRING) {
        tl_fetching_error("Incorrect magic of type String", "Incorrect magic of type String: %08x", magic);
        return tl_str_;
      }
    }
    return f$fetch_string();
  }
};

using t_String = tl_String_impl<false>;
using t_String_$ = tl_String_impl<true>;

struct t_Bool {
  void store(const var &tl_object) {
    if (tl_object.is_array()) {
      const var &name = tl_arr_get(tl_object, tl_str_underscore, 0, tl_str_underscore_hash);
      if (!name.is_null()) {
        const string &s = name.to_string();
        if (s == tl_str_boolFalse) {
          f$store_int(TL_BOOL_FALSE);
        } else if (s == tl_str_boolTrue) {
          f$store_int(TL_BOOL_TRUE);
        } else {
          tl_storing_error(tl_object, "Constructor %s of type Bool was not found in TL scheme", s.c_str());
          return;
        }
      }
    } else {
      f$store_int(tl_object.to_bool() ? TL_BOOL_TRUE : TL_BOOL_FALSE);
    }
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
        tl_fetching_error("Incorrect magic of type Bool", "Incorrect magic of type Bool: %08x", magic);
        return -1;
      }
    }
  }
};

template<typename T, bool is_bare>
struct tl_Vector_impl {
  T param_type;

  explicit tl_Vector_impl(T param_type) :
    param_type(std::move(param_type)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected array, got %s", v.get_type_c_str());
      return;
    }
    if (!is_bare) {
      f$store_int(TL_VECTOR);
    }
    const array<var> &a = v.as_array();
    int n = v.count();
    f$store_int(n);
    for (int i = 0; i < n; ++i) {
      if (!a.isset(i)) {
        tl_storing_error(v, "Vector[%d] not set", i);
        return;
      }
      param_type.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_VECTOR) {
        tl_fetching_error("Incorrect magic of type Vector", "Incorrect magic of type Vector: %08x", magic);
        return array<var>();
      }
    }
    int n = f$fetch_int();
    if (n < 0) {
      tl_fetching_error("Vector size is negative", "Vector size is negative");
      return array<var>();
    }
    array<var> result(array_size(std::min(n, 10000), 0, true));
    for (int i = 0; i < n; ++i) {
      const var &elem = param_type.fetch();
      CHECK_EXCEPTION(return result);
      result.push_back(elem);
    }
    return result;
  }
};

template<typename T>
using t_Vector = tl_Vector_impl<T, false>;
template<typename T>
using t_Vector_$ = tl_Vector_impl<T, true>;

template<typename T>
struct t_Maybe {
  T param_type;

  explicit t_Maybe(T param_type) :
    param_type(std::move(param_type)) {}

  void store(const var &v) {
    const string &name = f$strval(tl_arr_get(v, tl_str_underscore, 0, tl_str_underscore_hash));
    if (name == tl_str_resultFalse) {
      f$store_int(TL_RESULT_FALSE);
    } else if (name == tl_str_resultTrue) {
      f$store_int(TL_RESULT_TRUE);
      param_type.store(tl_arr_get(v, tl_str_result, 1, tl_str_result_hash));
    } else {
      tl_storing_error(v, "Constructor %s of type Maybe was not found in TL scheme", name.c_str());
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
        return param_type.fetch();
      }
      default: {
        tl_fetching_error("Incorrect magic of type Maybe", "Incorrect magic of type Maybe: %08x", magic);
        return -1;
      }
    }
  }
};

template<typename KeyT, typename ValueT, bool is_bare, unsigned int dict_magic>
struct tl_Dictionary_impl {
  ValueT value_type;

  explicit tl_Dictionary_impl(ValueT value_type) :
    value_type(std::move(value_type)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected dictionary, got something strange");
      return;
    }
    if (!is_bare) {
      f$store_int(dict_magic);
    }
    int n = v.count();
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().store(it.get_key());
      value_type.store(it.get_value());
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != dict_magic) {
        tl_fetching_error("Incorrect magic of type Dictionary", "Incorrect magic of type Dictionary: %08x", magic);
        return array<var>();
      }
    }
    array<var> result;
    int n = f$fetch_int();
    if (n < 0) {
      tl_fetching_error("Dictionary size is negative", "Dictionary size is negative");
      return result;
    }
    for (int i = 0; i < n; ++i) {
      const auto &key = KeyT().fetch();
      const var &value = value_type.fetch();
      CHECK_EXCEPTION(return result);
      result.set_value(key, value);
    }
    return result;
  }
};

template<typename T>
using t_Dictionary = tl_Dictionary_impl<t_String_$, T, false, TL_DICTIONARY>;
template<typename T>
using t_Dictionary_$ = tl_Dictionary_impl<t_String_$, T, true, TL_DICTIONARY>;

template<typename T>
using t_IntKeyDictionary = tl_Dictionary_impl<t_Int_$, T, false, TL_INT_KEY_DICTIONARY>;
template<typename T>
using t_IntKeyDictionary_$ = tl_Dictionary_impl<t_Int_$, T, true, TL_INT_KEY_DICTIONARY>;

template<typename T>
using t_LongKeyDictionary = tl_Dictionary_impl<t_Long_$, T, false, TL_LONG_KEY_DICTIONARY>;
template<typename T>
using t_LongKeyDictionary_$ = tl_Dictionary_impl<t_Long_$, T, true, TL_LONG_KEY_DICTIONARY>;

template<typename T, bool is_bare>
struct tl_Tuple_impl {
  T param_type;
  int size;

  tl_Tuple_impl(T param_type, int size) :
    param_type(std::move(param_type)),
    size(size) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected tuple, got %s", v.get_type_c_str());
      return;
    }
    if (!is_bare) {
      f$store_int(TL_TUPLE);
    }
    const array<var> &a = v.as_array();
    for (int i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        tl_storing_error(v, "Tuple[%d] not set", i);
        return;
      }
      param_type.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    if (!is_bare) {
      auto magic = static_cast<unsigned int>(f$fetch_int());
      if (magic != TL_TUPLE) {
        tl_fetching_error("Incorrect magic of type Tuple", "Incorrect magic of type Tuple: %08x", magic);
        return array<var>();
      }
    }
    array<var> result(array_size(size, 0, true));
    for (int i = 0; i < size; ++i) {
      result.push_back(param_type.fetch());
    }
    return result;
  }
};

template<typename T>
using t_Tuple = tl_Tuple_impl<T, false>;
template<typename T>
using t_Tuple_$ = tl_Tuple_impl<T, true>;

template<typename Cell>
struct tl_array {
  int size;
  Cell cell;

  tl_array(int size, Cell cell) :
    size(size),
    cell(std::move(cell)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    for (int i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        tl_storing_error(v, "Array[%d] not set", i);
        return;
      }
      cell.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    array<var> result(array_size(size, 0, true));
    CHECK_EXCEPTION(return result);
    for (int i = 0; i < size; ++i) {
      result.push_back(cell.fetch());
      CHECK_EXCEPTION(return result);
    }
    return result;
  }
};

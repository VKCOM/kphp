#pragma once

#include <memory>

#include "runtime/include.h"
#include "runtime/interface.h"
#include "runtime/rpc.h"

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

const string tl_str_("");
const string tl_str_underscore("_");
const string tl_str_resultFalse("resultFalse");
const string tl_str_resultTrue("resultTrue");
const string tl_str_result("result");

const int tl_str_underscore_hash = string_hash("_", 1);
const int tl_str_result_hash = string_hash("result", 6);

inline std::string dump_tl_array(const var &v) {
  do_var_dump(v, 0);
  string to_print = f$ob_get_contents();
  f$ob_clean();
  return to_print.c_str();
}

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

inline void store_magic_if_not_bare(unsigned int inner_magic) {
  if (inner_magic) {
    f$store_int(inner_magic);
  }
}

inline void fetch_magic_if_not_bare(unsigned int inner_magic, const char *error_msg) {
  if (inner_magic) {
    auto actual_magic = static_cast<unsigned int>(f$fetch_int());
    if (actual_magic != inner_magic) {
      tl_fetching_error("%s\nExpected 0x%08x, but fetched 0x%08x", error_msg, inner_magic, actual_magic);
    }
  }
}

struct t_Int {
  void store(const var &tl_object) {
    f$store_int(f$safe_intval(tl_object));
  }

  int fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_int();
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
};

struct t_Double {
  void store(const var &tl_object) {
    f$store_double(f$floatval(tl_object));
  }

  double fetch() {
    CHECK_EXCEPTION(return 0);
    return f$fetch_double();
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
        tl_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
        return -1;
      }
    }
  }
};

template<typename T, unsigned int inner_magic>
struct t_Vector {
  T param_type;

  explicit t_Vector(T param_type) :
    param_type(std::move(param_type)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected array, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    int n = v.count();
    f$store_int(n);
    for (int i = 0; i < n; ++i) {
      if (!a.isset(i)) {
        tl_storing_error(v, "Vector[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      param_type.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    int n = f$fetch_int();
    if (n < 0) {
      tl_fetching_error("Vector size is negative");
      return array<var>();
    }
    array<var> result(array_size(std::min(n, 10000), 0, true));
    for (int i = 0; i < n; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Vector");
      const var &elem = param_type.fetch();
      CHECK_EXCEPTION(return result);
      result.push_back(elem);
    }
    return result;
  }
};

template<typename T, unsigned int inner_magic>
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
      store_magic_if_not_bare(inner_magic);
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
        fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Maybe");
        return param_type.fetch();
      }
      default: {
        tl_fetching_error("Incorrect magic of type Maybe: 0x%08x", magic);
        return -1;
      }
    }
  }
};

template<typename KeyT, typename ValueT, unsigned int inner_value_magic>
struct tl_Dictionary_impl {
  ValueT value_type;

  explicit tl_Dictionary_impl(ValueT value_type) :
    value_type(std::move(value_type)) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected array (dictionary), got something strange");
      return;
    }
    int n = v.count();
    f$store_int(n);
    for (auto it = v.begin(); it != v.end(); ++it) {
      KeyT().store(it.get_key());
      store_magic_if_not_bare(inner_value_magic);
      value_type.store(it.get_value());
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    array<var> result;
    int n = f$fetch_int();
    if (n < 0) {
      tl_fetching_error("Dictionary size is negative");
      return result;
    }
    for (int i = 0; i < n; ++i) {
      const auto &key = KeyT().fetch();
      fetch_magic_if_not_bare(inner_value_magic, "Incorrect magic of inner type of some Dictionary");
      const var &value = value_type.fetch();
      CHECK_EXCEPTION(return result);
      result.set_value(key, value);
    }
    return result;
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
  T param_type;
  int size;

  t_Tuple(T param_type, int size) :
    param_type(std::move(param_type)),
    size(size) {}

  void store(const var &v) {
    if (!v.is_array()) {
      tl_storing_error(v, "Expected tuple, got %s", v.get_type_c_str());
      return;
    }
    const array<var> &a = v.as_array();
    for (int i = 0; i < size; ++i) {
      if (!a.isset(i)) {
        tl_storing_error(v, "Tuple[%d] not set", i);
        return;
      }
      store_magic_if_not_bare(inner_magic);
      param_type.store(v.get_value(i));
    }
  }

  array<var> fetch() {
    CHECK_EXCEPTION(return array<var>());
    array<var> result(array_size(size, 0, true));
    for (int i = 0; i < size; ++i) {
      fetch_magic_if_not_bare(inner_magic, "Incorrect magic of inner type of type Tuple");
      result.push_back(param_type.fetch());
    }
    return result;
  }
};

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

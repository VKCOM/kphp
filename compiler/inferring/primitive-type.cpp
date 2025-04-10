// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/primitive-type.h"

#include <map>
#include <string>

#include "common/algorithms/find.h"

#include "compiler/stage.h"

const char* ptype_name(PrimitiveType id) {
  switch (id) {
  case tp_any:
    return "any";
  case tp_Null:
    return "Null";
  case tp_False:
    return "False";
  case tp_bool:
    return "bool";
  case tp_int:
    return "int";
  case tp_float:
    return "float";
  case tp_array:
    return "array";
  case tp_string:
    return "string";
  case tp_tmp_string:
    return "tmp_string";
  case tp_mixed:
    return "mixed";
  case tp_tuple:
    return "tuple";
  case tp_shape:
    return "shape";
  case tp_future:
    return "future";
  case tp_future_queue:
    return "future_queue";
  case tp_regexp:
    return "regexp";
  case tp_Class:
    return "Class";
  case tp_object:
    return "object";
  case tp_void:
    return "void";
  case tp_Error:
    return "Error";
  case ptype_size:
    kphp_fail();
  }
  kphp_fail();
}

bool can_store_false(PrimitiveType tp) {
  kphp_assert(vk::none_of_equal(tp, tp_False, tp_Null));
  // any+or_false means just 'false' keyword, it's not wrapped into Optional: it's bool in cpp, but stays just false in type comparisons
  return vk::any_of_equal(tp, tp_bool, tp_mixed, tp_any);
}

bool can_store_null(PrimitiveType tp) {
  kphp_assert(vk::none_of_equal(tp, tp_False, tp_Null));
  // any+or_null means just 'null' keyword, it's represented as Optional<bool> in cpp, so any can't store null
  return vk::any_of_equal(tp, tp_mixed, tp_Class);
}

PrimitiveType type_lca(PrimitiveType a, PrimitiveType b) {
  kphp_assert(vk::none_of_equal(a, tp_False, tp_Null));
  kphp_assert(vk::none_of_equal(b, tp_False, tp_Null));

  if (a == b) {
    return a;
  }
  if (a == tp_Error || b == tp_Error) {
    return tp_Error;
  }

  if (a > b) {
    std::swap(a, b);
  }

  if (a == tp_any) {
    return b;
  }

  if (b >= tp_void) { // instances, future, etc — can mix only with false
    if (a == tp_Class && b == tp_object) {
      return tp_object;
    }
    return tp_Error;
  }

  if (vk::any_of_equal(a, tp_int, tp_float) && vk::any_of_equal(b, tp_int, tp_float)) { // float can store int
    return tp_float;
  }

  if (tp_bool <= a && a <= tp_mixed && tp_bool <= b && b <= tp_mixed) {
    return tp_mixed;
  }

  return std::max(a, b);
}

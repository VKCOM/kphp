// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/primitive-type.h"

#include <map>
#include <string>

#include "common/algorithms/find.h"

#include "compiler/stage.h"

const char *ptype_name(PrimitiveType id) {
  switch (id) {
    case tp_Unknown:       return "Unknown";
    case tp_Null:          return "Null";
    case tp_False:         return "False";
    case tp_bool:          return "bool";
    case tp_int:           return "int";
    case tp_float:         return "float";
    case tp_array:         return "array";
    case tp_string:        return "string";
    case tp_mixed:         return "mixed";
    case tp_tuple:         return "tuple";
    case tp_shape:         return "shape";
    case tp_future:        return "future";
    case tp_future_queue:  return "future_queue";
    case tp_regexp:        return "regexp";
    case tp_Class:         return "Class";
    case tp_void:          return "void";
    case tp_Error:         return "Error";
    case tp_Any:           return "Any";
    case tp_CreateAny:     return "CreateAny";
    case ptype_size:       kphp_fail();
  }
  kphp_fail();
}

bool can_store_false(PrimitiveType tp) {
  kphp_assert(vk::none_of_equal(tp, tp_False, tp_Null));
  return vk::any_of_equal(tp, tp_bool, tp_mixed, tp_Any);
}

bool can_store_null(PrimitiveType tp) {
  kphp_assert(vk::none_of_equal(tp, tp_False, tp_Null));
  return vk::any_of_equal(tp, tp_mixed, tp_Class, tp_Any);
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

  if (a == tp_Any) {
    return tp_Any;
  }
  if (b == tp_Any) {
    return a;
  }

  if (b == tp_CreateAny) {
    return tp_Any;
  }

  if (a > b) {
    std::swap(a, b);
  }

  if (a == tp_Unknown) {
    return b;
  }

  if (b >= tp_void) { // instances, future, etc — can mix only with false
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

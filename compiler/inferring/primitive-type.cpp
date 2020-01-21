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
    case tp_var:           return "var";
    case tp_UInt:          return "UInt";
    case tp_Long:          return "Long";
    case tp_ULong:         return "ULong";
    case tp_tuple:         return "tuple";
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
  return vk::any_of_equal(tp, tp_bool, tp_var, tp_Any);
}

bool can_store_null(PrimitiveType tp) {
  kphp_assert(vk::none_of_equal(tp, tp_False, tp_Null));
  return vk::any_of_equal(tp, tp_var, tp_Class, tp_Any);
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

  if ((b == tp_UInt || b == tp_Long || b == tp_ULong) && a != tp_int) { // UInt, Long and ULong can store only int
    return tp_Error;
  }

  if (tp_int <= a && a <= tp_float && tp_int <= b && b <= tp_float) { // float can store int
    return tp_float;
  }

  if (tp_bool <= a && a <= tp_var && tp_bool <= b && b <= tp_var) {
    return tp_var;
  }

  return std::max(a, b);
}

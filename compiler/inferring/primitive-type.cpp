#include "compiler/inferring/primitive-type.h"

#include <map>
#include <string>

#include "common/algorithms/find.h"

#include "compiler/stage.h"

std::map<std::string, PrimitiveType> name_to_ptype;

void get_ptype_by_name_init() {
  for (int tp_id = 0; tp_id < ptype_size; tp_id++) {
    auto tp = static_cast<PrimitiveType>(tp_id);
    name_to_ptype[ptype_name(tp)] = tp;
  }
}

PrimitiveType get_ptype_by_name(const std::string &s) {
  static bool inited = false;
  if (!inited) {
    get_ptype_by_name_init();
    inited = true;
  }
  auto it = name_to_ptype.find(s);
  if (it == name_to_ptype.end()) {
    return tp_Error;
  }
  return it->second;
}

const char *ptype_name(PrimitiveType id) {
  switch (id) {
    case tp_Unknown:    return "Unknown";
    case tp_False:      return "False";
    case tp_bool:       return "bool";
    case tp_int:        return "int";
    case tp_float:      return "float";
    case tp_array:      return "array";
    case tp_string:     return "string";
    case tp_var:        return "var";
    case tp_UInt:       return "UInt";
    case tp_Long:       return "Long";
    case tp_ULong:      return "ULong";
    case tp_RPC:        return "RPC";
    case tp_tuple:      return "tuple";
    case tp_regexp:     return "regexp";
    case tp_Class:      return "Class";
    case tp_void:       return "void";
    case tp_Error:      return "Error";
    case tp_Any:        return "Any";
    case tp_CreateAny:  return "CreateAny";
    case ptype_size:    kphp_fail();
  }
  kphp_fail();
}

bool can_store_bool(PrimitiveType tp) {
  return vk::any_of_equal(tp, tp_var, tp_Class, tp_RPC, tp_bool, tp_Any);
}


PrimitiveType type_lca(PrimitiveType a, PrimitiveType b) {
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

  if (b >= tp_RPC && a >= tp_int) { // Memcache and e.t.c can store only bool
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

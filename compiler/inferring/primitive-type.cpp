#include "compiler/inferring/primitive-type.h"

#include <map>
#include <string>

std::map<std::string, PrimitiveType> name_to_ptype;

void get_ptype_by_name_init() {
#define FOREACH_PTYPE(tp) name_to_ptype[PTYPE_NAME(tp)] = tp;

#include "../foreach_ptype.h"
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
#define FOREACH_PTYPE(tp) case tp: return PTYPE_NAME (tp);

#include "../foreach_ptype.h"

    default:
      return nullptr;
  }
}

bool can_store_bool(PrimitiveType tp) {
  return tp == tp_var || tp == tp_MC || tp == tp_DB || tp == tp_Class ||
         tp == tp_Exception || tp == tp_RPC || tp == tp_bool ||
         tp == tp_Any;
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

  if (b >= tp_MC && a >= tp_int) { // Memcache and e.t.c can store only bool
    return tp_Error;
  }

  if (b == tp_void) { // void can't store anything (except void)
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

// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/public.h"

struct LValue {
  tinf::Node* value;
  const MultiKey* key;
  LValue()
      : value(nullptr),
        key(nullptr) {}

  LValue(tinf::Node* value, const MultiKey* key)
      : value(value),
        key(key) {}

  LValue(const LValue& from)
      : value(from.value),
        key(from.key) {}

  LValue& operator=(const LValue& from) {
    value = from.value;
    key = from.key;
    return *this;
  }
};

LValue as_lvalue(VertexPtr v);

inline LValue as_lvalue(FunctionPtr function, int id) {
  return {tinf::get_tinf_node(function, id), &MultiKey::any_key(0)};
}

inline LValue as_lvalue(VarPtr var) {
  return {tinf::get_tinf_node(var), &MultiKey::any_key(0)};
}

inline const LValue& as_lvalue(const LValue& lvalue) {
  return lvalue;
}

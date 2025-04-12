// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/class-data.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/public.h"

struct RValue {
  RValue() = default;

  explicit RValue(const TypeData *type, const MultiKey *key = nullptr) :
    type(type),
    key(key) {
  }

  explicit RValue(tinf::Node *node, const MultiKey *key = nullptr) :
    node(node),
    key(key) {
  }

  const TypeData *type{nullptr};
  tinf::Node *node{nullptr};
  const MultiKey *key{nullptr};
  bool drop_or_false{false};
  bool drop_or_null{false};
  TypeData::FFIRvalueFlags ffi_flags;
};

// take &T cdata type and return it as T;
// for non-cdata types it has no effect
inline RValue ffi_rvalue_drop_ref(RValue rvalue) {
  rvalue.ffi_flags.drop_ref = true;
  return rvalue;
}

// take T cdata type and return it as *T;
// for non-cdata types it has no effect
inline RValue ffi_rvalue_take_addr(RValue rvalue) {
  rvalue.ffi_flags.take_addr = true;
  return rvalue;
}

inline RValue drop_optional(RValue rvalue) {
  rvalue.drop_or_false = true;
  rvalue.drop_or_null = true;
  return rvalue;
}

inline RValue drop_or_null(RValue rvalue) {
  rvalue.drop_or_null = true;
  return rvalue;
}

inline RValue drop_or_false(RValue rvalue) {
  rvalue.drop_or_false = true;
  return rvalue;
}


inline RValue as_rvalue(VertexPtr v, const MultiKey *key = nullptr) {
  return RValue(tinf::get_tinf_node(v), key);
}

inline RValue as_rvalue(FunctionPtr function, int param_i) {
  return RValue(tinf::get_tinf_node(function, param_i));
}

inline RValue as_rvalue(VarPtr var) {
  return RValue(tinf::get_tinf_node(var));
}

inline RValue as_rvalue(const TypeData *type, const MultiKey *key = nullptr) {
  return RValue(type, key);
}

inline const RValue &as_rvalue(const RValue &rvalue) {
  return rvalue;
}

inline RValue as_rvalue(tinf::Node *node, const MultiKey *key = nullptr) {
  return RValue(node, key);
}

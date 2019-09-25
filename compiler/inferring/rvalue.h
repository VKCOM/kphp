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
  bool drop_optional{false};
};


inline RValue drop_optional(RValue rvalue) {
  rvalue.drop_optional = true;
  return rvalue;
}

inline RValue as_rvalue(PrimitiveType primitive_type) {
  return RValue(TypeData::get_type(primitive_type));
}

inline RValue as_rvalue(VertexPtr v, const MultiKey *key = nullptr) {
  return RValue(tinf::get_tinf_node(v), key);
}

inline RValue as_rvalue(FunctionPtr function, int id) {
  return RValue(tinf::get_tinf_node(function, id));
}

inline RValue as_rvalue(VarPtr var) {
  return RValue(tinf::get_tinf_node(var));
}

inline RValue as_rvalue(ClassPtr klass) {
  return RValue(klass->type_data);
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

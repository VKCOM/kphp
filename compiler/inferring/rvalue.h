#pragma once

#include "compiler/data/class-data.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/public.h"

struct RValue {
  const TypeData *type;
  tinf::Node *node;
  const MultiKey *key;
  bool drop_or_false;
  RValue() :
    type(nullptr),
    node(nullptr),
    key(nullptr),
    drop_or_false(false) {
  }

  explicit RValue(const TypeData *type, const MultiKey *key = nullptr) :
    type(type),
    node(nullptr),
    key(key),
    drop_or_false(false) {
  }

  explicit RValue(tinf::Node *node, const MultiKey *key = nullptr) :
    type(nullptr),
    node(node),
    key(key),
    drop_or_false(false) {
  }

  RValue(const RValue &from) :
    type(from.type),
    node(from.node),
    key(from.key),
    drop_or_false(from.drop_or_false) {
  }

  RValue &operator=(const RValue &from) {
    type = from.type;
    node = from.node;
    key = from.key;
    drop_or_false = from.drop_or_false;
    return *this;
  }
};


inline RValue drop_or_false(RValue rvalue) {
  rvalue.drop_or_false = true;
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
  return RValue(klass->get_type_data());
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

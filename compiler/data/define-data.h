#pragma once

#include "common/algorithms/find.h"
#include "common/mixin/not_copyable.h"

#include "compiler/data/class-member-modifiers.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class DefineData : private vk::not_copyable {
public:
  enum DefineType {
    def_unknown,
    def_const,
    def_var
  };

  int id;
  DefineType type_;
  ClassPtr class_id; // class defining this constant; empty for non-class constants
  AccessModifiers access = AccessModifiers::public_;

  VertexPtr val;
  std::string name;
  SrcFilePtr file_id;

  DefineData();
  DefineData(std::string name, VertexPtr val, DefineType type_);

  inline DefineType &type() { return type_; }

};

// some operations can't contain define() expressions and they can be recursively skipped in FunctionPassBase;
// it helps to filter out a lot of code that doesn't need to be processed by this pass
inline bool can_define_be_inside_op(Operation type) {
  return vk::none_of_equal(type, op_set, op_func_call, op_while, op_for, op_foreach);
}

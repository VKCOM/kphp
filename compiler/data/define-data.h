// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/find.h"
#include "common/mixin/not_copyable.h"

#include "compiler/data/class-member-modifiers.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"

class DefineData : private vk::not_copyable {
  DEBUG_STRING_METHOD { return name; }
  
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
  ModulitePtr modulite;

  DefineData();
  DefineData(std::string name, VertexPtr val, DefineType type_);

  inline DefineType &type() { return type_; }
  std::string as_human_readable() const;

  bool is_builtin() const;
};

// some operations can't contain define() expressions and they can be recursively skipped in FunctionPassBase;
// it helps to filter out a lot of code that doesn't need to be processed by this pass
inline bool can_define_be_inside_op(Operation type) {
  return vk::none_of_equal(type, op_set, op_func_call, op_while, op_for, op_foreach);
}

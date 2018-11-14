#pragma once

#include "common/algorithms/find.h"

#include "compiler/data/data_ptr.h"

class DefineData {
public:
  enum DefineType {
    def_unknown,
    def_const,
    def_var
  };

  int id;
  DefineType type_;

  VertexPtr val;
  std::string name;
  SrcFilePtr file_id;

  DefineData();
  DefineData(VertexPtr val, DefineType type_);

  inline DefineType &type() { return type_; }

private:
  DISALLOW_COPY_AND_ASSIGN (DefineData);
};

// внутри некоторых операций не может существовать дефайнов, в них не заходим в FunctionPassBase рекурсивно
// т.е. отсекаем весьма много кода
inline bool can_define_be_inside_op(Operation type) {
  return vk::none_of_equal(type, op_set, op_func_call, op_constructor_call, op_while, op_for, op_foreach);
}

#pragma once

#include "compiler/function-pass.h"

class CheckModificationsOfConstFields : public FunctionPassBase {
public:
  string get_description() final {
    return "Check modifications of const fields";
  }

  bool check_function(FunctionPtr function) final {
    return default_check_function(function) &&
           function->root->type() == op_function &&
           function->type != FunctionData::func_class_holder;
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *);

  bool need_recursion(VertexPtr, LocalT *) {
    return !stage::has_error();
  }

private:
  void check_modification_of_const_class_field(VertexPtr v, bool write_flag = false) const;
};


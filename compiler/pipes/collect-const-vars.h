#pragma once

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass : public FunctionPassBase {
private:
  int in_param_list;

  int get_dependency_level(VertexPtr vertex);
  bool should_convert_to_const(VertexPtr root);
  VertexPtr create_const_variable(VertexPtr root, Location loc);

public:
  struct LocalT : public FunctionPassBase::LocalT {
    bool need_recursion_flag;
  };

  CollectConstVarsPass() :
    in_param_list(0) {
  }

  string get_description() {
    return "Collect constants";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  bool need_recursion(VertexPtr root __attribute__((unused)), LocalT *local);

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused)));

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (v->type() == op_function) {
      if (current_function->type() == FunctionData::func_class_holder) {
        ClassPtr c = current_function->class_id;
        c->members.for_each([&](ClassMemberStaticField &field) {
          if (field.init_val) {
            visit(field.init_val);
          }
        });
      }
    }
    return false;
  }

};

#pragma once

#include "compiler/function-pass.h"

class FinalCheckPass : public FunctionPassBase {
private:
  AUTO_PROF (final_check);
  int from_return;
public:

  string get_description() {
    return "Final check";
  }

  void init();

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local __attribute__((unused)));

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (v->type() == op_function) {
      visit(v.as<op_function>()->cmd());
      return true;
    }

    if (v->type() == op_func_call || v->type() == op_var ||
        v->type() == op_index || v->type() == op_constructor_call) {
      if (v->rl_type == val_r) {
        const TypeData *type = tinf::get_type(v);
        if (type->get_real_ptype() == tp_Unknown) {
          string index_depth;
          while (v->type() == op_index) {
            v = v.as<op_index>()->array();
            index_depth += "[.]";
          }
          string desc = "Using Unknown type : ";
          if (v->type() == op_var) {
            VarPtr var = v->get_var_id();
            desc += "variable [$" + var->name + "]" + index_depth;
          } else if (v->type() == op_func_call) {
            desc += "function [" + v.as<op_func_call>()->get_func_id()->name + "]" + index_depth;
          } else if (v->type() == op_constructor_call) {
            desc += "constructor [" + v.as<op_constructor_call>()->get_func_id()->name + "]" + index_depth;
          } else {
            desc += "...";
          }

          kphp_error (0, desc.c_str());
          return true;
        }
      }
    }

    return false;
  }

  VertexPtr on_exit_vertex(VertexPtr vertex, LocalT *local __attribute__((unused)));

private:
  void check_op_func_call(VertexAdaptor<op_func_call> call);
};


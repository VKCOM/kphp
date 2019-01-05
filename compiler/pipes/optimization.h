#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class OptimizationPass : public FunctionPassBase {
private:
  VertexPtr optimize_set_push_back(VertexAdaptor<op_set> set_op);
  void collect_concat(VertexPtr root, vector<VertexPtr> *collected);
  VertexPtr optimize_string_building(VertexPtr root);
  VertexPtr optimize_postfix_inc(VertexPtr root);
  VertexPtr optimize_postfix_dec(VertexPtr root);
  VertexPtr optimize_index(VertexAdaptor<op_index> index);
  template<Operation FromOp, Operation ToOp>
  VertexPtr fix_int_const(VertexPtr from, const string &from_func);
  VertexPtr fix_int_const(VertexPtr root);
  VertexPtr remove_extra_conversions(VertexPtr root);

public:
  string get_description() {
    return "Optimization";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);

  template<class VisitT>
  bool user_recursion(VertexPtr root, LocalT *local __attribute__((unused)), VisitT &visit) {
    if (root->type() == op_var) {
      VarPtr var = root->get_var_id();
      kphp_assert (var);
      if (var->init_val) {
        if (try_optimize_var(var)) {
          visit(var->init_val);
        }
      }
    }
    return false;
  }
};

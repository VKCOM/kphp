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

  bool check_function(FunctionPtr function);
  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);

  bool user_recursion(VertexPtr root, LocalT *local __attribute__((unused)), VisitVertex<OptimizationPass> &visit);
};

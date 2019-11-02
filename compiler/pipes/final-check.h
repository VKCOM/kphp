#pragma once

#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/inferring/public.h"

class FinalCheckPass : public FunctionPassBase {
private:
  void check_static_var_inited(VarPtr static_var);
  
public:

  string get_description() {
    return "Final check";
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local __attribute__((unused)));

  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitVertex<FinalCheckPass> &visit);

  VertexPtr on_exit_vertex(VertexPtr vertex, LocalT *local __attribute__((unused)));

private:
  void check_op_func_call(VertexAdaptor<op_func_call> call);
  void check_lib_exported_function(FunctionPtr function);
  void check_eq3_neq3(VertexPtr lhs, VertexPtr rhs, const char *op_str);
  void check_eq2_neq2(VertexPtr lhs, VertexPtr rhs, const char *op_str);
};


#pragma once

#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/inferring/public.h"

class FinalCheckPass final : public FunctionPassBase {
private:
  void check_static_var_inited(VarPtr static_var);
  
public:

  string get_description() override {
    return "Final check";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  bool on_start(FunctionPtr function) override;

  VertexPtr on_enter_vertex(VertexPtr vertex) override;

  bool user_recursion(VertexPtr v) override;

  VertexPtr on_exit_vertex(VertexPtr vertex) override;

private:
  void check_op_func_call(VertexAdaptor<op_func_call> call);
  void check_lib_exported_function(FunctionPtr function);
  void check_eq3_neq3(VertexPtr lhs, VertexPtr rhs, Operation op);
  void check_comparisons(VertexPtr lhs, VertexPtr rhs, Operation op);
  void raise_error_using_Unknown_type(VertexPtr v);
};


#pragma once

#include "compiler/function-pass.h"

class ExtractResumableCallsPass : public FunctionPassBase {
private:
  static void skip_conv_and_sets(VertexPtr *&replace);
  static VertexPtr *get_resumable_func_for_replacement(VertexPtr vertex);
  static VertexAdaptor<op_var> make_temp_resumable_var(const TypeData *type);

public:
  string get_description() {
    return "Extract easy resumable calls";
  }

  bool check_function(FunctionPtr function);

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;
  };

  void on_enter_edge(VertexPtr vertex, LocalT *local __attribute__((unused)), VertexPtr dest_vertex __attribute__((unused)), LocalT *dest_local);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local);
};

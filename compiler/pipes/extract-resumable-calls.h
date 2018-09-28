#pragma once

#include "compiler/function-pass.h"

class ExtractResumableCallsPass : public FunctionPassBase {
private:
  AUTO_PROF (extract_resumable_calls);

  void skip_conv_and_sets(VertexPtr *&replace);

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

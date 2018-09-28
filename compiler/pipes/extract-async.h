#pragma once

#include "compiler/function-pass.h"

class ExtractAsyncPass : public FunctionPassBase {
private:
  AUTO_PROF (extract_async);
public:
  string get_description() {
    return "Extract async";
  }

  bool check_function(FunctionPtr function);

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;
  };

  void on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, LocalT *dest_local);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local);
};

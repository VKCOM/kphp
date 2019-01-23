#pragma once


#include "compiler/function-pass.h"

class CommonAnalyzerPass : public FunctionPassBase {
  void check_set(VertexAdaptor<op_set> to_check);
  void analyzer_check_array(VertexPtr to_check);

public:

  string get_description() {
    return "Try to detect common errors";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && !function->is_extern();
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool from_seq;

    LocalT() :
      from_seq() {}
  };

  void on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr , LocalT *dest_local);

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT * local);
};

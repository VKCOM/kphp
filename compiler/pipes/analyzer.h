#pragma once


#include "compiler/function-pass.h"

class CommonAnalyzerPass : public FunctionPassBase {
  void check_set(VertexAdaptor<op_set> to_check);

public:

  string get_description() {
    return "Try to detect common errors";
  }

  bool check_function(FunctionPtr function) {
    return !function->is_extern();
  }


  VertexPtr on_enter_vertex(VertexPtr vertex);

  std::nullptr_t on_finish();
};

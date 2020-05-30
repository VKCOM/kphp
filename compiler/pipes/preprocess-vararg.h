#pragma once

#include "compiler/function-pass.h"

class PreprocessVarargPass final : public FunctionPassBase {
private:
  VertexAdaptor<op_var> create_va_list_var(Location loc);

public:
  string get_description() override {
    return "Preprocess variadic functions";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;

  bool check_function(FunctionPtr function) override;
};

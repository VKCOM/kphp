#pragma once

#include "compiler/function-pass.h"

class PreprocessVarargPass : public FunctionPassBase {
private:
  AUTO_PROF (preprocess_break);

  VertexPtr create_va_list_var(Location loc);


public:

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);

  virtual bool check_function(FunctionPtr function);
};

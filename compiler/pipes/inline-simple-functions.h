#pragma once

#include "compiler/function-pass.h"

class InlineSimpleFunctions : public FunctionPassBase {
private:
  bool inline_is_possible_{true};

public:
  std::string get_description() final { return "Inline simple functions"; }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);

  bool check_function(FunctionPtr function) final;
  std::nullptr_t on_finish();
};

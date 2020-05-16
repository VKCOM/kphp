#pragma once

#include "compiler/function-pass.h"

class ResolveSelfStaticParentPass : public FunctionPassBase {
private:
  void check_access_to_class_from_this_file(ClassPtr ref_class);
public:
  string get_description() {
    return "Resolve self/static/parent";
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr v);
};

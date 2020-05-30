#pragma once

#include "compiler/function-pass.h"

class ResolveSelfStaticParentPass final : public FunctionPassBase {
private:
  void check_access_to_class_from_this_file(ClassPtr ref_class);
public:
  string get_description() override {
    return "Resolve self/static/parent";
  }

  bool on_start(FunctionPtr function) override;

  VertexPtr on_enter_vertex(VertexPtr v) override;
};

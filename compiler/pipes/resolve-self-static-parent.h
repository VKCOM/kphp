// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ResolveSelfStaticParentPass final : public FunctionPassBase {
private:
  void check_access_to_class_from_this_file(ClassPtr ref_class);
public:
  string get_description() override {
    return "Resolve self/static/parent";
  }

  void on_start() override;

  VertexPtr on_enter_vertex(VertexPtr v) override;
};

// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

/*** Calculate const_type for all nodes ***/
class CalcConstTypePass final : public FunctionPassBase {
  void calc_const_type_of_class_fields(ClassPtr klass);
  int inlined_define_cnt{0};
public:

  void on_start() override;

  std::string get_description() override {
    return "Calc const types";
  }

  VertexPtr on_exit_vertex(VertexPtr v) override;
  VertexPtr on_enter_vertex(VertexPtr v) override;
};

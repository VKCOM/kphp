// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once


#include "compiler/function-pass.h"

class CommonAnalyzerPass final : public FunctionPassBase {
  void check_set(VertexAdaptor<op_set> to_check);

public:

  std::string get_description() override {
    return "Try to detect common errors";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }


  VertexPtr on_enter_vertex(VertexPtr vertex) override;

  void on_finish() override;
};

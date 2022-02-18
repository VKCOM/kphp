// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class PreprocessBreakPass final : public FunctionPassBase {
private:
  std::vector<VertexAdaptor<meta_op_cycle>> cycles;
  int current_label_id{0};

  int get_label_id(VertexAdaptor<meta_op_cycle> cycle, Operation op);

public:
  std::string get_description() override {
    return "Preprocess breaks";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;

  VertexPtr on_exit_vertex(VertexPtr root) override;
};

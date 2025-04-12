// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class PreprocessEq3Pass final : public FunctionPassBase {
private:
  VertexPtr convert_eq3_null_to_isset(VertexPtr eq_op, VertexPtr not_null);
public:
  std::string get_description() override {
    return "Preprocess eq3";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;
};

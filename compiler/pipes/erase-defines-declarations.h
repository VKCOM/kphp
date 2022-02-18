// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/define-data.h"
#include "compiler/function-pass.h"

class EraseDefinesDeclarationsPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Erase defines declarations";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr v) override {
    return !can_define_be_inside_op(v->type());
  }
};

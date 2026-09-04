// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/data/define-data.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/function-pass.h"
#include "compiler/vertex-meta_op_base.h"

class RegisterDefinesPass final : public FunctionPassBase {

public:
  std::string get_description() override {
    return "Register defines";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr v) override {
    return !can_define_be_inside_op(v->type());
  }
};

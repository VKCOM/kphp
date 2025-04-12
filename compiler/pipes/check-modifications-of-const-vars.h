// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckModificationsOfConstVars final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Check modifications of const fields";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;

  bool user_recursion(VertexPtr) override {
    return stage::has_error();
  }

private:
  void check_modifications(VertexPtr v, bool write_flag = false) const;
};


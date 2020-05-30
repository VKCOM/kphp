#pragma once

#include "compiler/function-pass.h"

class CheckModificationsOfConstVars : public FunctionPassBase {
public:
  string get_description() final {
    return "Check modifications of const fields";
  }

  VertexPtr on_enter_vertex(VertexPtr v);

  bool user_recursion(VertexPtr) {
    return stage::has_error();
  }

private:
  void check_modifications(VertexPtr v, bool write_flag = false) const;
};


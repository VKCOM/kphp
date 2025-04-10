// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/propagate-throw-flag.h"

// todo this pass bubbles throw_flag up, but throw_flag is used only in one place in codegenerator
// investigate, maybe it can be totally avoided?

VertexPtr PropagateThrowFlagPass::on_enter_vertex(VertexPtr v) {
  if (v->type() == op_callback_of_builtin) {
    v->throw_flag = v.as<op_callback_of_builtin>()->func_id->can_throw();
  } else if (v->type() == op_func_call) {
    v->throw_flag = v.as<op_func_call>()->func_id->can_throw();
  }

  return v;
}

VertexPtr PropagateThrowFlagPass::on_exit_vertex(VertexPtr v) {
  for (auto child : *v) {
    if (child->throw_flag) {
      v->throw_flag = true;
      break;
    }
  }

  return v;
}

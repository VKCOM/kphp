// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/fix-returns.h"

#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

VertexPtr FixReturnsPass::on_enter_vertex(VertexPtr root) {
  auto is_void_fun = [](FunctionPtr f) {
    return tinf::get_type(f, -1)->ptype() == tp_void;
  };
  auto is_void_expr = [](VertexPtr root) {
    return tinf::get_type(root)->ptype() == tp_void;
  };

  if (root->rl_type == val_r && is_void_expr(root)) {
    if (auto call = root.try_as<op_func_call>()) {
      FunctionPtr fun = call->func_id;
      kphp_error(0, fmt_format("Using result of void function {}", fun->as_human_readable()));
    } else {
      if (root->type() == op_throw) {
        kphp_error(false, TermStringFormat::paint("throw expression ", TermStringFormat::blue) + "isn't supported");
      }
      kphp_error(0, "Using result of void expression");
    }
  }

  if (root->rl_type == val_none) {
    if (auto call = root.try_as<op_func_call>()) {
      FunctionPtr f = call->func_id;
      if (f->warn_unused_result) {
        kphp_error(false, "Result of function call is unused, but function is marked with @kphp-warn-unused-result");
      }
    }
  }

  if (auto return_op = root.try_as<op_return>()) {
    if (is_void_fun(current_function) && return_op->has_expr() && is_void_expr(return_op->expr())) {
      std::vector<VertexPtr> seq;
      seq.push_back(return_op->expr());
      seq.back()->rl_type = val_none;
      seq.push_back(VertexAdaptor<op_return>::create());
      seq.back()->location = return_op->location;
      return VertexAdaptor<op_seq>::create(seq);
    }
    return return_op;
  }

  return root;
}

bool FixReturnsPass::user_recursion(VertexPtr root) {
  if (auto fork_vertex = root.try_as<op_fork>()) {
    for (auto arg : fork_vertex->func_call()->args()) {
      run_function_pass(arg, this);
    }
    return true;
  }
  return false;
}

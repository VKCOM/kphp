// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-actual-edges.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

// handle_exception reports whether a thrown_class will be caught by any of the given try blocks;
// try_stack should contain the innermost try block in its end.
//
// This function will mark try blocks that failed to handle the exception with catches_all=false.
bool CalcActualCallsEdgesPass::handle_exception(std::vector<VertexAdaptor<op_try>> &try_stack, ClassPtr thrown_class) {
  // walk from the innermost try statement
  for (auto i = try_stack.rbegin(); i != try_stack.rend(); ++i) {
    auto &try_op = *i;
    for (auto c : try_op->catch_list()) {
      if (c.as<op_catch>()->exception_class->is_parent_of(thrown_class)) {
        return true;
      }
    }
    try_op->catches_all = false;
  }
  return false;
}

VertexPtr CalcActualCallsEdgesPass::on_enter_vertex(VertexPtr v) {
  if (auto ptr = v.try_as<op_func_ptr>()) {
    if (ptr->func_id->is_lambda()) {
      ClassPtr lambda_class = ptr->func_id->class_id;
      lambda_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
        if (!m.function->is_template) {
          edges.emplace_back(m.function, try_stack_, inside_fork);
        }
      });
    } else {
      edges.emplace_back(ptr->func_id, try_stack_, inside_fork);
    }
  } else if (auto call = v.try_as<op_func_call>()) {
    edges.emplace_back(call->func_id, try_stack_, inside_fork);
  }

  if (v->type() == op_throw) {
    auto as_instance = infer_class_of_expr(stage::get_function(), v.as<op_throw>()->exception()).try_as<AssumInstance>();
    // TODO: an error message that is more actionable, like in name-gen.cpp (i.e. suggest to add @return, etc.)
    kphp_error_act(as_instance, "Throw expression is not an instance or it can't be detected", return v);
    ClassPtr thrown_class = as_instance->klass;
    if (!handle_exception(try_stack_, thrown_class)) {
      current_function->exceptions_thrown.insert(thrown_class);
      current_function->throws_location = v->location;
    }
  }

  return v;
}

bool CalcActualCallsEdgesPass::user_recursion(VertexPtr v) {
  if (auto try_v = v.try_as<op_try>()) {
    try_stack_.push_back(try_v);
    run_function_pass(try_v->try_cmd_ref(), this);
    try_stack_.pop_back();
    for (auto c : try_v->catch_list()) {
      auto catch_op = c.as<op_catch>();
      run_function_pass(catch_op->cmd_ref(), this);
    }
    return true;
  }
  if (auto fork_v = v.try_as<op_fork>()) {
    inside_fork++;
    run_function_pass(fork_v->func_call_ref(), this);
    inside_fork--;
    return true;
  }
  return false;
}

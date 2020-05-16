#include "compiler/pipes/calc-actual-edges.h"

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

VertexPtr CalcActualCallsEdgesPass::on_enter_vertex(VertexPtr v) {
  if (auto ptr = v.try_as<op_func_ptr>()) {
    if (ptr->func_id->is_lambda()) {
      ClassPtr lambda_class = ptr->func_id->class_id;
      lambda_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
        if (!m.function->is_template) {
          edges.emplace_back(m.function, inside_try, inside_fork);
        }
      });
    } else {
      edges.emplace_back(ptr->func_id, inside_try, inside_fork);
    }
  } else if (auto call = v.try_as<op_func_call>()) {
    edges.emplace_back(call->func_id, inside_try, inside_fork);
  }
  if (v->type() == op_throw && !inside_try) {
    current_function->can_throw = true;
  }
  return v;
}

bool CalcActualCallsEdgesPass::user_recursion(VertexPtr v, VisitVertex<CalcActualCallsEdgesPass> &visit) {
  if (auto try_v = v.try_as<op_try>()) {
    inside_try++;
    visit(try_v->try_cmd_ref());
    inside_try--;
    visit(try_v->catch_cmd_ref());
    return true;
  }
  if (auto fork_v = v.try_as<op_fork>()) {
    inside_fork++;
    visit(fork_v->func_call_ref());
    inside_fork--;
    return true;
  }
  return false;
}

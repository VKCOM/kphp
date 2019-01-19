#include "compiler/pipes/calc-actual-edges.h"

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

VertexPtr CalcActualCallsEdgesPass::on_enter_vertex(VertexPtr v, LocalT *) {
  if (vk::any_of_equal(v->type(), op_func_call, op_constructor_call, op_func_ptr)) {
    if (v->type() == op_func_ptr && v->get_func_id()->is_lambda()) {
      ClassPtr lambda_class = v->get_func_id()->class_id;
      lambda_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
        if (!m.function->is_template) {
          edges.emplace_back(m.function, inside_try);
        }
      });
    } else {
      edges.emplace_back(v->get_func_id(), inside_try);
    }
  }
  if (v->type() == op_throw && !inside_try) {
    current_function->root->throws_flag = true;
  }
  return v;
}

#include "compiler/pipes/remove-empty-function-calls.h"

#include "compiler/vertex.h"

VertexPtr RemoveEmptyFunctionCalls::on_enter_vertex(VertexPtr v, LocalT *) {
  if (v->type() == op_func_call && v->get_func_id()->body_seq == FunctionData::body_value::empty) {
    return VertexAdaptor<op_null>::create();
  }
  return v;
}

VertexPtr RemoveEmptyFunctionCalls::on_exit_vertex(VertexPtr v, LocalT *) {
  if (vk::any_of_equal(v->type(), op_require, op_require_once)) {
    kphp_assert_msg(v->size() == 1, "Only one child possible for require vertex");
    if (v->back()->type() == op_null) {
      if (v->type() == op_require) {
        return VertexAdaptor<op_null>::create();
      } else {
        return VertexAdaptor<op_empty>::create();
      }
    }
  }
  return v;
}

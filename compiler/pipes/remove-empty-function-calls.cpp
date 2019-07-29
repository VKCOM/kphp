#include "compiler/pipes/remove-empty-function-calls.h"

#include "compiler/vertex.h"

VertexPtr RemoveEmptyFunctionCalls::on_exit_vertex(VertexPtr v, LocalT *) {
  if (v->type() == op_func_call && v.as<op_func_call>()->func_id->body_seq == FunctionData::body_value::empty) {
    return VertexAdaptor<op_null>::create();
  }
  if (v->type() == op_seq) {
    auto is_empty = [](VertexPtr v) { return v->type() == op_null || v->type() == op_empty;};
    if (std::any_of(v->begin(), v->end(), is_empty)) {
      std::vector<VertexPtr> next;
      std::remove_copy_if(v->begin(), v->end(), std::back_inserter(next), is_empty);
      return VertexAdaptor<op_seq>::create(next);
    }
  }
  if (v->type() == op_if) {
    auto if_v = v.as<op_if>();
    auto cond = if_v->cond();
    if (cond->type() == op_log_not && cond.as<op_log_not>()->expr()->type() == op_var && !if_v->has_false_cmd()) {
      if (if_v->true_cmd()->type() == op_seq && if_v->true_cmd()->size() == 0) {
        return VertexAdaptor<op_empty>::create();
      }
    }
  }
  return v;
}

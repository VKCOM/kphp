#include "compiler/pipes/extract-async.h"

bool ExtractAsyncPass::check_function(FunctionPtr function) {
  return default_check_function(function) && !function->is_extern() && function->is_resumable;
}
void ExtractAsyncPass::on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, ExtractAsyncPass::LocalT *dest_local) {
  dest_local->from_seq = vk::any_of_equal(vertex->type(), op_seq, op_seq_rval);
}
VertexPtr ExtractAsyncPass::on_enter_vertex(VertexPtr vertex, ExtractAsyncPass::LocalT *local) {
  if (!local->from_seq) {
    return vertex;
  }
  VertexAdaptor<op_func_call> func_call = vertex.try_as<op_func_call>();
  VertexPtr lhs;
  if (!func_call) {
    if (auto set = vertex.try_as<op_set>()) {
      if (auto rhs = set->rhs().try_as<op_func_call>()) {
        func_call = rhs;
        lhs = set->lhs();
      }
    }
  }
  if (!func_call) {
    return vertex;
  }
  FunctionPtr func = func_call->func_id;
  if (!func->is_resumable) {
    return vertex;
  }
  VertexAdaptor<op_async> async;
  if (!lhs) {
    async = VertexAdaptor<op_async>::create(func_call);
  } else {
    if (kphp_error (lhs->type() == op_var, "Can't save result of resumable function into non-var inside another resumable function")) {
      return vertex;
    }
    async = VertexAdaptor<op_async>::create(func_call, lhs.as<op_var>());
  }
  async.set_location(func_call);
  return async;
}

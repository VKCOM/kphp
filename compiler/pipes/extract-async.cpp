#include "compiler/pipes/extract-async.h"

bool ExtractAsyncPass::check_function(FunctionPtr function) {
  return default_check_function(function) && !function->is_extern() && function->is_resumable;
}
void ExtractAsyncPass::on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, ExtractAsyncPass::LocalT *dest_local) {
  dest_local->from_seq = vertex->type() == op_seq || vertex->type() == op_seq_rval;
}
VertexPtr ExtractAsyncPass::on_enter_vertex(VertexPtr vertex, ExtractAsyncPass::LocalT *local) {
  if (local->from_seq == false) {
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
  if (!lhs) {
    auto empty = VertexAdaptor<op_empty>::create();
    set_location(empty, vertex->get_location());
    lhs = empty;
  }
  auto async = VertexAdaptor<op_async>::create(lhs, func_call);
  set_location(async, func_call->get_location());
  return async;
}

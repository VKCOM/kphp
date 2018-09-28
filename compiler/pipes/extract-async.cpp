#include "compiler/pipes/extract-async.h"

bool ExtractAsyncPass::check_function(FunctionPtr function) {
  return default_check_function(function) && function->type() != FunctionData::func_extern &&
         function->root->resumable_flag;
}
void ExtractAsyncPass::on_enter_edge(VertexPtr vertex, LocalT *, VertexPtr, ExtractAsyncPass::LocalT *dest_local) {
  dest_local->from_seq = vertex->type() == op_seq || vertex->type() == op_seq_rval;
}
VertexPtr ExtractAsyncPass::on_enter_vertex(VertexPtr vertex, ExtractAsyncPass::LocalT *local) {
  if (local->from_seq == false) {
    return vertex;
  }
  VertexAdaptor<op_func_call> func_call;
  VertexPtr lhs;
  if (vertex->type() == op_func_call) {
    func_call = vertex;
  } else if (vertex->type() == op_set) {
    VertexAdaptor<op_set> set = vertex;
    VertexPtr rhs = set->rhs();
    if (rhs->type() == op_func_call) {
      func_call = rhs;
      lhs = set->lhs();
    }
  }
  if (!func_call) {
    return vertex;
  }
  FunctionPtr func = func_call->get_func_id();
  if (func->root->resumable_flag == false) {
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

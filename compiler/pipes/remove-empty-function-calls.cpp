#include "compiler/pipes/remove-empty-function-calls.h"

#include "compiler/vertex.h"

VertexPtr RemoveEmptyFunctionCalls::on_enter_vertex(VertexPtr v, LocalT *) {
  if (v->type() == op_func_call && v->get_func_id()->body_seq == FunctionData::body_value::empty) {
    return VertexAdaptor<op_null>::create();
  }
  return v;
}

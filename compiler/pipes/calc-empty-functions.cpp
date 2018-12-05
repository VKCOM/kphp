#include "compiler/pipes/calc-empty-functions.h"

#include "compiler/data/function-data.h"
#include "compiler/function-pass.h"
#include "compiler/vertex.h"

namespace {

FunctionData::body_value calc_function_body_type(FunctionPtr f);
FunctionData::body_value calc_seq_body_type(VertexAdaptor<op_seq> seq);

FunctionData::body_value get_vertex_body_type(VertexPtr vertex) {
  switch (vertex->type()) {
    case op_require:
    case op_require_once:
    case op_func_call:
      return FunctionData::body_value::unknown;
    case op_return: {
      VertexAdaptor<op_return> return_vertex = vertex;
      return return_vertex->expr()->type() == op_null
             ? FunctionData::body_value::empty
             : get_vertex_body_type(return_vertex->expr());
    }
    case op_seq:
      return calc_seq_body_type(vertex.as<op_seq>());
    case op_empty:
      return FunctionData::body_value::empty;
    default:
      return FunctionData::body_value::non_empty;
  }
}

FunctionData::body_value calc_seq_body_type(VertexAdaptor<op_seq> seq) {
  FunctionData::body_value result = FunctionData::body_value::empty;
  for (VertexPtr seq_element: *seq) {
    FunctionData::body_value vertex_body = get_vertex_body_type(seq_element);
    if (vertex_body == FunctionData::body_value::non_empty) {
      return FunctionData::body_value::non_empty;
    }
    if (vertex_body == FunctionData::body_value::unknown) {
      result = FunctionData::body_value::unknown;
    }
  }
  return result;
}

FunctionData::body_value calc_function_body_type(FunctionPtr f) {
  if (f->body_seq != FunctionData::body_value::unknown) {
    return f->body_seq;
  }

  if (f->type() == FunctionData::func_extern ||
      f->type() == FunctionData::func_switch ||
      f->root->type() != op_function ||
      !f->root.as<op_function>()->params()->empty()) {
    f->body_seq = FunctionData::body_value::non_empty;
  } else {
    f->body_seq = calc_seq_body_type(f->root.as<op_function>()->cmd());
  }
  return f->body_seq;
}

} // anonymous namespace


void CalcEmptyFunctions::execute(FunctionPtr f, DataStream<FunctionPtr> &os) {
  calc_function_body_type(f);
  os << f;
}

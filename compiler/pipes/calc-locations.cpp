#include "compiler/pipes/calc-locations.h"

VertexPtr CalcLocationsPass::on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *) {
  stage::set_line(v->location.line);
  v->location = stage::get_location();
  return v;
}

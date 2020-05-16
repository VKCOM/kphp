#include "compiler/pipes/calc-locations.h"

#include "compiler/data/class-data.h"

bool CalcLocationsPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  if (function->type == FunctionData::func_class_holder) {
    function->class_id->members.for_each([](ClassMemberConstant &constant) {
      stage::set_line(constant.value->location.line);
      constant.value.set_location(stage::get_location());
    });
  }
  return true;
}

VertexPtr CalcLocationsPass::on_enter_vertex(VertexPtr v) {
  stage::set_line(v->location.line);
  v->location = stage::get_location();
  return v;
}

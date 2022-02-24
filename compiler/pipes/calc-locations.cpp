// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-locations.h"

#include "compiler/data/class-data.h"

void CalcLocationsPass::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([](ClassMemberInstanceField &f) {
      stage::set_line(f.root->location.line);
      f.root->location = stage::get_location();
    });
    current_function->class_id->members.for_each([](ClassMemberStaticField &f) {
      stage::set_line(f.root->location.line);
      f.root->location = stage::get_location();
    });
    current_function->class_id->members.for_each([](ClassMemberConstant &c) {
      stage::set_line(c.value->location.line);
      c.value.set_location(stage::get_location());
    });
  }
}

VertexPtr CalcLocationsPass::on_enter_vertex(VertexPtr v) {
  stage::set_line(v->location.line);
  v->location = stage::get_location();
  return v;
}

// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-locations.h"

#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"

void CalcLocationsPass::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([this](ClassMemberInstanceField &f) {
      f.root->location = Location{current_function->file_id, current_function, f.root->location.line};
      if (f.var->init_val) {
        f.var->init_val.set_location_recursively(f.root->location);
      }
    });
    current_function->class_id->members.for_each([this](ClassMemberStaticField &f) {
      f.root->location = Location{current_function->file_id, current_function, f.root->location.line};
      if (f.var->init_val) {
        f.var->init_val.set_location_recursively(f.root->location);
      }
    });
    current_function->class_id->members.for_each([this](ClassMemberConstant &c) {
      c.value.set_location_recursively(Location{current_function->file_id, current_function, c.value->location.line});
    });
  }
}

VertexPtr CalcLocationsPass::on_enter_vertex(VertexPtr v) {
  stage::set_line(v->location.line);
  v->location = stage::get_location();
  return v;
}

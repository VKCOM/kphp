// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-magic-const.h"

#include "compiler/data/class-data.h"

VertexPtr CalcMagicConstPass::on_enter_vertex(VertexPtr root) {
  if (root->type() == op_class_c) {
    auto cur_class = current_function->class_id;
    auto class_name = cur_class ? cur_class->name : "";

    auto r = VertexAdaptor<op_string>::create();
    r->location = root->get_location();
    r->str_val = std::move(class_name);

    return r;
  }

  return root;
}

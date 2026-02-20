// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-magic-const.h"

#include "compiler/data/class-data.h"

VertexPtr CalcMagicConstPass::on_enter_vertex(VertexPtr root) {
  if (root->type() == op_class_c) {
    auto cur_class = current_function->class_id;
    auto class_name = cur_class ? cur_class->name : "";

    auto trait_name = VertexAdaptor<op_string>::create();
    trait_name->location = root->get_location();
    trait_name->str_val = std::move(class_name);

    return trait_name;
  }

  return root;
}

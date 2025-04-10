// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-defines.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

VertexPtr RegisterDefinesPass::on_exit_vertex(VertexPtr root) {
  // 1. an explicit define('name', value)
  if (auto define = root.try_as<op_define>()) {
    VertexPtr name = define->name();
    VertexPtr val = define->value();

    kphp_error_act(name->type() == op_string, "Define name should be a valid string", return root);

    auto* data = new DefineData(name->get_string(), val, DefineData::def_unknown);
    data->file_id = stage::get_file();
    G->register_define(DefinePtr(data));

    // we keep the define() call for now; if it'll end up being a constant,
    // it will be removed in EraseDefinesDeclarationsPass
  }

  // 2. class constants. They're stored outside of the AST
  if (root->type() == op_function && current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->register_defines();
  }

  return root;
}

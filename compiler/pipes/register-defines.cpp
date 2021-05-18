// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-defines.h"

#include "compiler/compiler-core.h"
#include "compiler/name-gen.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

VertexPtr RegisterDefinesPass::on_exit_vertex(VertexPtr root) {
  // 1. an explicit define('name', value)
  if (auto define = root.try_as<op_define>()) {
    VertexPtr val = define->value();

    SrcFilePtr file = stage::get_file();
    auto name = gen_define_name(file, define);
    printf("DEFINE %s\n", name.c_str());

    auto *data = new DefineData(name, val, DefineData::def_unknown);
    data->file_id = file;
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

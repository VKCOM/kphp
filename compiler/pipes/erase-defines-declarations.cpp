// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/erase-defines-declarations.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/define-data.h"

VertexPtr EraseDefinesDeclarationsPass::on_exit_vertex(VertexPtr root) {
  // when inside a function:
  // define('NAME', constval) -> nothing (const values will be inlined)
  // define('NAME', f())      -> d$NAME = f()
  if (auto define_op = root.try_as<op_define>()) {
    DefinePtr define = G->get_define(define_op->name()->get_string());

    if (define->type() == DefineData::def_var) {
      auto var = VertexAdaptor<op_var>::create().set_location(root);
      var->extra_type = op_ex_var_superglobal;
      var->str_val = "d$" + define->name;
      var->is_const = true;

      auto new_root = VertexAdaptor<op_set>::create(var, define->val).set_location(root);
      root = new_root;
    } else {
      root = VertexAdaptor<op_empty>::create();
    }
  }

  return root;
}

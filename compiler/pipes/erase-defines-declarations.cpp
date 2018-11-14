#include "compiler/pipes/erase-defines-declarations.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/define-data.h"

VertexPtr EraseDefinesDeclarationsPass::on_exit_vertex(VertexPtr root, LocalT *) {
  // define('NAME', 1) внутри функций превратить в ничто для константных дефайнов — они заинлайнятся
  // а define('NAME', f()) — превратить в d$NAME = f()
  if (root->type() == op_define) {
    VertexAdaptor<op_define> define_op = root;
    DefinePtr define = G->get_define(define_op->name()->get_string());

    if (define->type() == DefineData::def_var) {
      auto var = VertexAdaptor<op_var>::create();
      var->extra_type = op_ex_var_superglobal;
      var->str_val = "d$" + define->name;
      set_location(var, root->get_location());

      auto new_root = VertexAdaptor<op_set>::create(var, define->val);
      set_location(new_root, root->get_location());
      root = new_root;
    } else {
      root = VertexAdaptor<op_empty>::create();
    }
  }

  return root;
}

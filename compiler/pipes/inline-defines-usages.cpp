#include "compiler/pipes/inline-defines-usages.h"

#include "compiler/data/define-data.h"

VertexPtr InlineDefinesUsagesPass::on_enter_vertex(VertexPtr root, LocalT *) {
  // defined('NAME') заменяем на true или false 
  if (root->type() == op_defined) {
    VertexAdaptor<op_defined> defined = root;

    kphp_error_act (
      (int)root->size() == 1 && defined->expr()->type() == op_string,
      "wrong arguments in 'defined'",
      return VertexPtr()
    );

    DefinePtr def = G->get_define(defined->expr()->get_string());

    if (def) {
      root = VertexAdaptor<op_true>::create();
    } else {
      root = VertexAdaptor<op_false>::create();
    }
  }

  // значения константных дефайнов заменяем на op_define_val, а неконстантных — на переменную d$...
  if (root->type() == op_func_name) {
    DefinePtr d = G->get_define(resolve_define_name(root->get_string()));
    if (d) {
      if (d->type() == DefineData::def_var) {
        auto var = VertexAdaptor<op_var>::create();
        var->extra_type = op_ex_var_superglobal;
        var->str_val = "d$" + d->name;
        root = var;
      } else {
        auto def = VertexAdaptor<op_define_val>::create();
        def->define_id = d;
        root = def;
      } 
    }
  }
  return root;
}

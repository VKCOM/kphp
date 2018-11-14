#include "compiler/pipes/register-defines.h"

#include "compiler/data/define-data.h"

VertexPtr RegisterDefinesPass::on_enter_vertex(VertexPtr root, LocalT *) {
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

  if (root->type() == op_func_name) {
    string name = root->get_string();
    name = resolve_define_name(name);
    DefinePtr d = G->get_define(name);
    if (d) {
      assert (d->name == name);
      if (d->type() == DefineData::def_var) {
        auto var = VertexAdaptor<op_var>::create();
        var->extra_type = op_ex_var_superglobal;
        var->str_val = "d$" + name;
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

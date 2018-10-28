#include "compiler/pipes/register-defines.h"

#include "compiler/data/define-data.h"

VertexPtr RegisterDefinesPass::on_enter_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_defined) {
    bool is_defined = false;

    VertexAdaptor<op_defined> defined = root;

    kphp_error_act (
      (int)root->size() == 1 && defined->expr()->type() == op_string,
      "wrong arguments in 'defined'",
      return VertexPtr()
    );

    const string name = defined->expr().as<op_string>()->str_val;
    DefinePtr def = G->get_define(name);
    is_defined = def && def->name == name;

    if (is_defined) {
      auto true_val = VertexAdaptor<op_true>::create();
      root = true_val;
    } else {
      auto false_val = VertexAdaptor<op_false>::create();
      root = false_val;
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
      } else if (d->type() == DefineData::def_raw || d->type() == DefineData::def_php) {
        auto def = VertexAdaptor<op_define_val>::create();
        def->define_id = d;
        root = def;
      } else {
        kphp_assert (0 && "unreachable branch");
      }
    }
  }
  return root;
}

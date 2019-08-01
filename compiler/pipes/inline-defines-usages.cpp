#include "compiler/pipes/inline-defines-usages.h"

#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/name-gen.h"

VertexPtr InlineDefinesUsagesPass::on_enter_vertex(VertexPtr root, LocalT *) {
  // defined('NAME') заменяем на true или false
  if (auto defined = root.try_as<op_defined>()) {
    kphp_error_act (
      (int)root->size() == 1 && defined->expr()->type() == op_string,
      "wrong arguments in 'defined'",
      return VertexPtr()
    );

    DefinePtr def = G->get_define(defined->expr()->get_string());

    if (def) {
      root = VertexAdaptor<op_true>::create().set_location(root);
    } else {
      root = VertexAdaptor<op_false>::create().set_location(root);
    }
  }

  // значения константных дефайнов заменяем на саму переменную, а неконстантных — на переменную d$...
  if (root->type() == op_func_name) {
    DefinePtr d = G->get_define(resolve_define_name(root->get_string()));
    if (d) {
      if (d->type() == DefineData::def_var) {
        auto var = VertexAdaptor<op_var>::create().set_location(root);
        var->extra_type = op_ex_var_superglobal;
        var->str_val = "d$" + d->name;
        root = var;
      } else {
        root = d->val.clone().set_location_recursively(root);
      } 
    }
  }
  return root;
}

bool InlineDefinesUsagesPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  if (function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](ClassMemberStaticField &f) {
      if (f.init_val) {
        f.init_val = run_function_pass(f.init_val, this, nullptr);
      }
    });
    current_function->class_id->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        f.var->init_val = run_function_pass(f.var->init_val, this, nullptr);
      }
    });
  }

  return true;
}

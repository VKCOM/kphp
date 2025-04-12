// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/inline-defines-usages.h"

#include "compiler/data/define-data.h"
#include "compiler/modulite-check-rules.h"
#include "compiler/name-gen.h"
#include "compiler/pipes/check-access-modifiers.h"

VertexPtr InlineDefinesUsagesPass::on_enter_vertex(VertexPtr root) {
  // defined('NAME') is replaced by true or false
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

  // const value defines are replaced by their value;
  // non-const defines are replaced by d$ variables
  if (root->type() == op_func_name) {
    std::string name = resolve_define_name(root->get_string());
    DefinePtr def = G->get_define(name);
    if (!def) {
      const auto readable_name = vk::replace_all(vk::replace_all(name, "$$", "::"), "$", "\\");
      kphp_error(0, fmt_format("Undefined constant '{}'", readable_name));
      return root;
    }

    if (current_function->modulite != def->modulite) {
      // When `class B extends A`, def=B::CONST would refer to A::CONST actually,
      // but we should perform all checks for B (requested_class), not for A
      if (name[0] == 'c' && name[1] == '#') {
        std::string requested_cn = root->get_string().substr(0, root->get_string().find("$$"));
        modulite_check_when_use_constant(current_function, def, G->get_class(requested_cn));
      } else {
        modulite_check_when_use_global_const(current_function, def);
      }
    }

    if (def->type() == DefineData::def_var) {
      auto var = VertexAdaptor<op_var>::create().set_location(root);
      var->extra_type = op_ex_var_superglobal;
      var->str_val = "d$" + def->name;
      root = var;
    } else {
      if (def->class_id) {
        auto access_class = def->class_id;
        check_access(class_id, lambda_class_id, FieldModifiers{def->access}, access_class, "const", def->as_human_readable());
      }
      auto wrapped = VertexAdaptor<op_define_val>::create(def->val.clone()).set_location_recursively(root);
      wrapped->define_id = def;
      root = wrapped;
    }
  }

  return root;
}

void InlineDefinesUsagesPass::on_start() {
  class_id = current_function->class_id;
  lambda_class_id = current_function->get_this_or_topmost_if_lambda()->class_id;

  if (current_function->type == FunctionData::func_class_holder) {
    current_function->class_id->members.for_each([&](ClassMemberStaticField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
    current_function->class_id->members.for_each([&](ClassMemberInstanceField &f) {
      if (f.var->init_val) {
        run_function_pass(f.var->init_val, this);
      }
    });
  }
}

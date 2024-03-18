// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-access-modifiers.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"

void CheckAccessModifiersPass::on_start() {
  lambda_class_id = current_function->get_this_or_topmost_if_lambda()->class_id;
  class_id = current_function->class_id;
}

VertexPtr CheckAccessModifiersPass::on_enter_vertex(VertexPtr root) {
  if (auto var = root.try_as<op_var>()) {
    VarPtr var_id = var->var_id;
    if (var_id->is_class_static_var()) {
      const auto *member = var_id->as_class_static_field();
      kphp_assert(member);
      check_access(class_id, lambda_class_id, member->modifiers, var_id->class_id, "static field", member->local_name());
    }
  } else if (auto prop = root.try_as<op_instance_prop>()) {
    VarPtr var_id = prop->var_id;
    if (var_id->is_class_instance_var()) {
      const auto *member = var_id->as_class_instance_field();
      kphp_assert(member);
      check_access(class_id, lambda_class_id, member->modifiers, var_id->class_id, "field", member->local_name());
    }
  } else if (auto call = root.try_as<op_func_call>()) {
    FunctionPtr func_id = call->func_id;
    if (func_id->modifiers.is_instance() || func_id->modifiers.is_static()) {
      // TODO: this is hack, which should be fixed after functions with context are added to static methods list
      auto real_name = func_id->local_name().substr(0, func_id->local_name().find("$$"));
      if (func_id->context_class->members.has_static_method(real_name)) {
        check_access(class_id, lambda_class_id, func_id->modifiers, func_id->class_id, "static method", func_id);
      } else if (func_id->context_class->members.has_instance_method(real_name)) {
        check_access(class_id, lambda_class_id, func_id->modifiers, func_id->class_id, "method", func_id);
      }
    }
  }

  return root;
}

#include "compiler/pipes/check-access-modifiers.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"

bool CheckAccessModifiersPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }
  lambda_class_id = function->get_this_or_topmost_if_lambda()->class_id;
  class_id = function->class_id;
  return true;
}

void CheckAccessModifiersPass::check_access(AccessType need_access, ClassPtr access_class, const char *field_type, const std::string &name) {
  if (need_access == access_static_private || need_access == access_private) {
    kphp_error(class_id == access_class || lambda_class_id == access_class, format("Can't access private %s %s", field_type, name.c_str()));
  }
  if (need_access == access_static_protected || need_access == access_protected) {
    auto is_ok = [&access_class](ClassPtr class_id) {
      return class_id && (class_id == access_class || class_id->is_parent_of(access_class) || access_class->is_parent_of(class_id));
    };
    kphp_error(is_ok(class_id) || is_ok(lambda_class_id),
               format("Can't access protected %s %s", field_type, name.c_str()));
  }
}

VertexPtr CheckAccessModifiersPass::on_enter_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_var) {
    VarPtr var_id = root->get_var_id();
    if (var_id->is_class_static_var()) {
      auto member = var_id->as_class_static_field();
      kphp_assert(member);
      check_access(member->access_type, var_id->class_id, "static field", member->local_name());
    }
  } else if (root->type() == op_instance_prop) {
    VarPtr var_id = root->get_var_id();
    if (var_id->is_class_instance_var()) {
      auto member = var_id->as_class_instance_field();
      kphp_assert(member);
      check_access(member->access_type, var_id->class_id, "field", member->local_name());
    }
  } else if (vk::any_of_equal(root->type(), op_func_call, op_constructor_call)) {
    FunctionPtr func_id = root.as<op_func_call>()->get_func_id();
    if (func_id->access_type != access_nonmember) {
      //TODO: this is hack, which should be fixed after functions with context are added to static methods list
      std::string real_name = func_id->local_name().substr(0, func_id->local_name().find("$$"));
      if (func_id->context_class->members.has_static_method(real_name)) {
        check_access(func_id->access_type, func_id->class_id, "static method", func_id->get_human_readable_name());
      } else if (func_id->context_class->members.has_instance_method(real_name)) {
        check_access(func_id->access_type, func_id->class_id, "method", func_id->get_human_readable_name());
      }
    }
  }

  return root;
}

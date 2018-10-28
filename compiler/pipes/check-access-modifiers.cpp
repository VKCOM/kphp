#include "compiler/pipes/check-access-modifiers.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"

bool CheckAccessModifiersPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }
  class_id = function->class_id ? function->get_outer_class() : ClassPtr();
  return true;
}
VertexPtr CheckAccessModifiersPass::on_enter_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_var) {
    VarPtr var_id = root.as<op_var>()->get_var_id();
    if (var_id->is_class_static_var()) {
      auto member = var_id->as_class_static_field();
      string name = var_id->name;
      size_t pos = name.find("$$");

      kphp_error(member->access_type != access_nonmember,
                 dl_pstr("Field wasn't declared: %s", member->local_name().c_str()));
      kphp_error(member->access_type != access_static_private ||
                 (class_id && replace_backslashes(class_id->name) == name.substr(0, pos)),
                 dl_pstr("Can't access private field %s", member->local_name().c_str()));
      if (member->access_type == access_static_protected) {
        kphp_error_act(class_id, dl_pstr("Can't access protected field %s", member->local_name().c_str()), return root);
        ClassPtr var_class = var_id->class_id;
        ClassPtr klass = G->get_class(class_id->name);      // это не class_id! из-за множественного парсинга одного файла
        while (klass && var_class != klass) {
          klass = klass->parent_class;
        }
        kphp_error(klass, dl_pstr("Can't access protected field %s", member->local_name().c_str()));
      }
    }
  } else if (root->type() == op_func_call) {
    FunctionPtr func_id = root.as<op_func_call>()->get_func_id();
    if (func_id->access_type != access_nonmember) {
      string name = func_id->name;
      size_t pos = name.find("$$");
      kphp_assert(pos != string::npos);
      kphp_error(func_id->access_type != access_static_private ||
                 (class_id && replace_backslashes(class_id->name) == name.substr(0, pos)),
                 dl_pstr("Can't access private function %s", name.c_str()));
      // TODO: check protected
    }
  }
  return root;
}

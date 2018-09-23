#include "compiler/pipes/check-access-modifiers.h"

#include "compiler/compiler-core.h"

bool CheckAccessModifiers::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }
  namespace_name = function->namespace_name;
  class_name = function->class_name;
  return true;
}
VertexPtr CheckAccessModifiers::on_enter_vertex(VertexPtr root, LocalT *) {
  kphp_assert (root);
  if (root->type() == op_var) {
    VarPtr var_id = root.as<op_var>()->get_var_id();
    string real_name = root.as<op_var>()->str_val;
    string name = var_id->name;
    size_t pos = name.find("$$");
    if (pos != string::npos) {
      kphp_error(var_id->access_type != access_nonmember,
                 dl_pstr("Field wasn't declared: %s", real_name.c_str()));
      kphp_error(var_id->access_type != access_static_private ||
                 replace_characters(namespace_name, '\\', '$') + "$" + class_name == name.substr(0, pos),
                 dl_pstr("Can't access private field %s", real_name.c_str()));
      if (var_id->access_type == access_static_protected) {
        kphp_error_act(!namespace_name.empty() && !class_name.empty(), dl_pstr("Can't access protected field %s", real_name.c_str()), return root);
        ClassPtr var_class = var_id->class_id;
        ClassPtr klass = G->get_class(namespace_name + "\\" + class_name);
        kphp_assert(klass);
        while (klass && var_class != klass) {
          klass = klass->parent_class;
        }
        kphp_error(klass, dl_pstr("Can't access protected field %s", real_name.c_str()));
      }
    }
  } else if (root->type() == op_func_call) {
    FunctionPtr func_id = root.as<op_func_call>()->get_func_id();
    string name = func_id->name;
    size_t pos = name.find("$$");
    if (pos != string::npos) {
      kphp_assert(func_id->access_type != access_nonmember);
      kphp_error(func_id->access_type != access_static_private ||
                 replace_backslashes(namespace_name) + "$" + class_name == name.substr(0, pos),
                 dl_pstr("Can't access private function %s", name.c_str()));
      // TODO: check protected
    }
  }
  return root;
}

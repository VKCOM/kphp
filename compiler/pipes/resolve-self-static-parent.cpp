#include "compiler/pipes/resolve-self-static-parent.h"
#include "compiler/name-gen.h"

VertexPtr ResolveSelfStaticParentPass::on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *) {
  // заменяем \VK\A::method, static::f, self::$field на полные имена вида VK$A$$method и пр.
  if (v->type() == op_func_call || v->type() == op_var || v->type() == op_func_name) {
    const string &name = v->get_string();
    if (name.find("::") != string::npos) {
      const std::string &member_name = get_full_static_member_name(current_function, name, v->type() == op_func_call);
      v->set_string(member_name);
    }
  }

  return v;
}

#include "compiler/pipes/resolve-self-static-parent.h"
#include "compiler/name-gen.h"
#include "compiler/data/class-data.h"

bool ResolveSelfStaticParentPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  // заменяем self::, parent:: и обращения к другим классам типа Classes\A::CONST внутри констант классов
  if (function->type() == FunctionData::func_class_wrapper) {
    current_function->class_id->members.for_each([&](ClassMemberConstant &c) {
      c.value = run_function_pass(c.value, this, nullptr);
    });
    current_function->class_id->members.for_each([&](ClassMemberStaticField &c) {
      c.init_val = run_function_pass(c.init_val, this, nullptr);
    });
  }
  return true;
}

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

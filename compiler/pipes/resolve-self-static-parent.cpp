#include "compiler/pipes/resolve-self-static-parent.h"

#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"

bool ResolveSelfStaticParentPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function)) {
    return false;
  }

  // заменяем self::, parent:: и обращения к другим классам типа Classes\A::CONST внутри констант классов
  if (function->type == FunctionData::func_class_holder) {
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
    size_t pos = name.find("::");
    if (pos != string::npos) {
      const std::string &member_name = get_full_static_member_name(current_function, name, v->type() == op_func_call);
      v->set_string(member_name);
      const std::string &class_name = resolve_uses(current_function, name.substr(0, pos), '\\');
      check_access_to_class_from_this_file(class_name);
    }
  }

  // заменяем new A на new Classes\A, т.е. зарезолвленное полное имя класса
  if (v->type() == op_constructor_call) {
    if (!v->get_func_id() && likely(!v->type_help)) {     // type_help <=> Memcache | Exception
      const std::string &class_name = resolve_uses(current_function, v->get_string(), '\\');
      v->set_string(class_name);
      check_access_to_class_from_this_file(class_name);
    }
  }

  return v;
}

inline void ResolveSelfStaticParentPass::check_access_to_class_from_this_file(const std::string &class_name) {
  ClassPtr ref_class = G->get_class(class_name);
  if (ref_class && !ref_class->can_be_php_autoloaded) {
    kphp_error(ref_class->file_id == current_function->file_id,
               format("Class %s can be accessed only from file %s, as it is not autoloadable",
                      ref_class->name.c_str(), ref_class->file_id->unified_file_name.c_str()));
  }
}

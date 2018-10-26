#include "compiler/pipes/check-infered-instances.h"

#include "compiler/data.h"

void CheckInferredInstancesPass::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check inferred instances");
  stage::set_function(function);

  if (function->type() != FunctionData::func_extern && !function->assumptions_for_vars.empty()) {
    analyze_function_vars(function);
  }
  if (function->class_id && function->class_id->init_function == function && function->class_id->was_constructor_invoked) {
    analyze_class(function->class_id);
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}
void CheckInferredInstancesPass::analyze_function_vars(FunctionPtr function) {
  auto analyze_vars = [this, function](const std::vector<VarPtr> &vars) {
    for (const auto &var: vars) {
      analyze_function_var(function, var);
    }
  };

  analyze_vars(function->local_var_ids);
  analyze_vars(function->global_var_ids);
  analyze_vars(function->static_var_ids);
}
void CheckInferredInstancesPass::analyze_function_var(FunctionPtr function, VarPtr var) {
  ClassPtr klass;
  AssumType assum = assumption_get_for_var(function, var->name, klass);

  if (assum == assum_instance) {
    const TypeData *t = var->tinf_node.get_type();
    kphp_error((t->ptype() == tp_Class && klass == t->class_type())
               || (t->ptype() == tp_Exception || t->ptype() == tp_MC),
               dl_pstr("var $%s assumed to be %s, but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(t).c_str()));
  } else if (assum == assum_instance_array) {
    const TypeData *t = var->tinf_node.get_type()->lookup_at(Key::any_key());
    kphp_error(t != nullptr && ((t->ptype() == tp_Class && klass == t->class_type())
                                || (t->ptype() == tp_Exception || t->ptype() == tp_MC)),
               dl_pstr("var $%s assumed to be %s[], but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(var->tinf_node.get_type()).c_str()));
  }
}

void CheckInferredInstancesPass::analyze_class(ClassPtr klass) {
  // при несовпадении phpdoc и выведенного типов — ошибка уже кинулась раньше, на этапе type inferring
  // а здесь мы проверим переменные классов, которые объявлены, но никогда не записывались и не имеют дефолтного значения
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_Unknown,
               dl_pstr("var %s::$%s is declared but never written", klass->name.c_str(), f.local_name().c_str()));
  });
}

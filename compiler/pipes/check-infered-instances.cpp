#include "compiler/pipes/check-infered-instances.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"

// todo функциональность этого пайпа должна потом переехать на restrictions (KPHP-204)
// (чтобы если assumptions по переменным не верны — был stacktrace и вот это всё)

void CheckInferredInstancesF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check inferred instances");

  if (function->type() != FunctionData::func_extern && !function->assumptions_for_vars.empty()) {
    stage::set_function(function);
    analyze_function_vars(function);
    if (stage::has_error()) {
      return;
    }
  }

  os << function;
}
void CheckInferredInstancesF::analyze_function_vars(FunctionPtr function) {
  auto analyze_vars = [this, function](const std::vector<VarPtr> &vars) {
    for (const auto &var: vars) {
      analyze_function_var(function, var);
    }
  };

  analyze_vars(function->local_var_ids);
  analyze_vars(function->global_var_ids);
  analyze_vars(function->static_var_ids);
}
void CheckInferredInstancesF::analyze_function_var(FunctionPtr function, VarPtr var) {
  ClassPtr klass;
  AssumType assum = assumption_get_for_var(function, var->name, klass);

  if (assum == assum_instance) {
    const TypeData *t = var->tinf_node.get_type();
    kphp_error((t->ptype() == tp_Class && klass == t->class_type())
               || (t->ptype() == tp_Exception || t->ptype() == tp_MC),
               format("var $%s assumed to be %s, but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(t).c_str()));
  } else if (assum == assum_instance_array) {
    const TypeData *t = var->tinf_node.get_type()->lookup_at(Key::any_key());
    kphp_error(t != nullptr && ((t->ptype() == tp_Class && klass == t->class_type())
                                || (t->ptype() == tp_Exception || t->ptype() == tp_MC)),
               format("var $%s assumed to be %s[], but inferred %s", var->name.c_str(), klass->name.c_str(), type_out(var->tinf_node.get_type()).c_str()));
  }
}

#include "compiler/pipes/check-classes.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"

void CheckClassesF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check classes");

  if (function->class_id && function->class_id->init_function == function && function->class_id->was_constructor_invoked) {
    stage::set_function(function);
    analyze_class(function->class_id);
    if (stage::has_error()) {
      return;
    }
  }

  os << function;
}

inline void CheckClassesF::analyze_class(ClassPtr klass) {
  check_static_fields_inited(klass);
  check_instance_fields_inited(klass);
}


inline void CheckClassesF::check_static_fields_inited(ClassPtr klass) {
  // todo KPHP-220
  (void)klass;
}

inline void CheckClassesF::check_instance_fields_inited(ClassPtr klass) {
  // todo KPHP-221 ; пока что оставлен старый вариант (проверка на Unknown)
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_Unknown,
               dl_pstr("var %s::$%s is declared but never written", klass->name.c_str(), f.local_name().c_str()));
  });
}

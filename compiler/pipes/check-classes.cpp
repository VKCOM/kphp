#include "compiler/pipes/check-classes.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/vertex.h"

void CheckClassesF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check classes");

  if (function->type() == FunctionData::func_class_wrapper) {
    stage::set_function(function);
    analyze_class(function->class_id);
    if (stage::has_error()) {
      return;
    }
  }

  os << function;
}

inline void CheckClassesF::analyze_class(ClassPtr klass) {
  //printf("Check class %s\n", klass->name.c_str());
  check_static_fields_inited(klass);
  if (klass->was_constructor_invoked) {
    check_instance_fields_inited(klass);
  }
}


/*
 * Проверяем, что все static-поля класса инициализированы при объявлении
 */
inline void CheckClassesF::check_static_fields_inited(ClassPtr klass) {
  klass->members.for_each([&](const ClassMemberStaticField &f) {
    bool allow_no_default_value = false;
    // если дефолтного значения нет — а вдруг оно не обязательно? для инстансов например
    if (!f.init_val) {
      allow_no_default_value = vk::any_of_equal(f.get_inferred_type()->ptype(), tp_Class, tp_MC);
    }

    kphp_error(f.init_val || allow_no_default_value,
               format("static %s::$%s is not inited at declaration (inferred %s)",
                       klass->name.c_str(), f.local_name().c_str(), colored_type_out(f.get_inferred_type()).c_str()));
  });
}

inline void CheckClassesF::check_instance_fields_inited(ClassPtr klass) {
  // todo KPHP-221 ; пока что оставлен старый вариант (проверка на Unknown)
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_Unknown,
               format("var %s::$%s is declared but never written", klass->name.c_str(), f.local_name().c_str()));
  });
}

#include "compiler/pipes/check-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/type-data.h"
#include "compiler/vertex.h"

void CheckClassesF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check classes");

  if (function->type == FunctionData::func_class_holder) {
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
  if (ClassData::does_need_codegen(klass)) {
    check_instance_fields_inited(klass);
  }
  if (klass->can_be_php_autoloaded && !klass->is_builtin()) {
    kphp_error(klass->file_id->main_function->body_seq == FunctionData::body_value::empty,
               fmt_format("class {} can be autoloaded, but its file contains some logic (maybe, require_once files with global vars?)\n",
                          klass->name));
  }
}


/*
 * Проверяем, что все static-поля класса инициализированы при объявлении
 */
inline void CheckClassesF::check_static_fields_inited(ClassPtr klass) {
  klass->members.for_each([&](const ClassMemberStaticField &f) {
    bool allow_no_default_value = false;
    // если дефолтного значения нет — а вдруг оно не обязательно? для инстансов например
    if (!f.var->init_val) {
      allow_no_default_value = vk::any_of_equal(f.get_inferred_type()->ptype(), tp_Class);
    }

    kphp_error(f.var->init_val || allow_no_default_value,
               fmt_format("static {}::${} is not inited at declaration (inferred {})",
                          klass->name, f.local_name(), colored_type_out(f.get_inferred_type())));
  });
}

inline void CheckClassesF::check_instance_fields_inited(ClassPtr klass) {
  // todo KPHP-221 ; пока что оставлен старый вариант (проверка на Unknown)
  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    PrimitiveType ptype = f.var->tinf_node.get_type()->get_real_ptype();
    kphp_error(ptype != tp_Unknown,
               fmt_format("var {}::${} is declared but never written; please, provide a default value", klass->name, f.local_name()));
    kphp_error(!(ptype == tp_Class && f.var->tinf_node.get_type()->class_type()->is_fully_static()),
               fmt_format("var {}::${} refers to fully static class", klass->name, f.local_name()));
  });
}

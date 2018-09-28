#include "compiler/pipes/collect-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data.h"
#include "compiler/name-gen.h"

void CollectClassF::execute(FunctionPtr data, DataStream<FunctionPtr> &os) {
  stage::set_name("Collect classes");

  if (data->class_id && data->class_id->init_function == data) {
    ClassPtr klass = data->class_id;
    if (!data->class_extends.empty()) {
      klass->extends = resolve_uses(data, data->class_extends, '\\');
    }
    if (!klass->extends.empty()) {
      klass->parent_class = G->get_class(klass->extends);
      kphp_assert(klass->parent_class);
      kphp_error(klass->is_fully_static() && klass->parent_class->is_fully_static(),
                 dl_pstr("Invalid class extends %s and %s: extends is available only if classes are only-static",
                         klass->name.c_str(), klass->parent_class->name.c_str()));
    } else {
      klass->parent_class = ClassPtr();
    }
  }
  os << data;
}

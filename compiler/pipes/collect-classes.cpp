#include "compiler/pipes/collect-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data.h"
#include "compiler/name-gen.h"

void CollectClassF::execute(FunctionPtr data, DataStream<FunctionPtr> &os) {
  stage::set_name("Collect classes");

  if (data->class_id && data->class_id->init_function == data) {
    ClassPtr klass = data->class_id;

    auto extends_it = std::find_if(klass->str_dependents.begin(), klass->str_dependents.end(),
                                   [](ClassData::StrDependence &dep) { return dep.type == ctype_class; });

    if (extends_it != klass->str_dependents.end()) {
      string extends_full_classname = resolve_uses(data, extends_it->class_name, '\\');
      klass->parent_class = G->get_class(extends_full_classname);
      kphp_assert(klass->parent_class);
      kphp_error(!klass->members.has_constructor() && !klass->parent_class->members.has_constructor(),
                 dl_pstr("Invalid class extends %s and %s: extends is available only if classes are only-static",
                         klass->name.c_str(), klass->parent_class->name.c_str()));
    }
  }
  
  os << data;
}

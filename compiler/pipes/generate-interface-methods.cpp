#include "compiler/pipes/generate-interface-methods.h"
#include "compiler/data/function-data.h"
#include "compiler/data/interface-generator.h"

void GenerateInterfaceMethods::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Generate interface methods of abstract classes");
  stage::set_function(function);
  kphp_assert(function);

  bool is_in_interface = function->class_id && function->class_id->is_interface();
  if (is_in_interface && function->type != FunctionData::func_class_holder) {
    kphp_error_return(!function->is_static_function(), "static functions are not allowed in interfaces");
    generate_body_of_interface_method(function);
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}

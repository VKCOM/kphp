#include "compiler/pipes/generate-virtual-methods.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/virtual-method-generator.h"

void GenerateVirtualMethods::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Generate virtual methods of interfaces/base classes");
  stage::set_function(function);
  kphp_assert(function);

  if (function->is_virtual_method) {
    generate_body_of_virtual_method(function, os);
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}

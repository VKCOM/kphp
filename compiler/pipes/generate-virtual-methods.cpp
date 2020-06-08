#include "compiler/pipes/generate-virtual-methods.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/virtual-method-generator.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/pipes/find-lambdas-with-interface-pass.h"

void GenerateVirtualMethods::execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os) {
  stage::set_name("Generate virtual methods of interfaces/base classes");
  stage::set_function(function);
  kphp_assert(function);

  if (function->is_virtual_method) {
    generate_body_of_virtual_method(function);
  } else {
    FindLambdasWithInterfacePass interface_finder;
    run_function_pass(function, &interface_finder);
    if (!interface_finder.lambdas_interfaces.empty()) {
      AutoLocker<Lockable *> locker(&mutex);
      for (const auto &interface_inheritors : interface_finder.lambdas_interfaces) {
        auto &res_inheritors = lambdas_interfaces[interface_inheritors.first];
        res_inheritors.insert(res_inheritors.end(), interface_inheritors.second.begin(), interface_inheritors.second.end());
      }
    }
  }

  if (stage::has_error()) {
    return;
  }

  Base::execute(function, unused_os);
}

void GenerateVirtualMethods::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();
  for (auto &interface_inheritors : lambdas_interfaces) {
    const auto &interface = interface_inheritors.first;
    auto &inheritors = interface_inheritors.second;
    std::sort(inheritors.begin(), inheritors.end());

    interface->derived_classes = inheritors;
    auto invoke_method = interface->get_instance_method(ClassData::NAME_OF_INVOKE_METHOD);
    kphp_assert(invoke_method);
    generate_body_of_virtual_method(invoke_method->function);

    for (auto &derived_lambda : interface->derived_classes) {
      if (derived_lambda->is_lambda()) {
        auto generated_virt_clone = derived_lambda->get_instance_method(ClassData::NAME_OF_VIRT_CLONE)->function;
        G->register_and_require_function(generated_virt_clone, this->tmp_stream, true);
      }
    }
  }

  Base::on_finish(os);
}

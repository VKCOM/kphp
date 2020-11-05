// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/lambda-interface-generator.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/lambda-generator.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/type-data.h"
#include "compiler/utils/string-utils.h"

InterfacePtr LambdaInterfaceGenerator::generate() noexcept {
  auto interface_name = generate_name();

  auto interface = generate_interface_class(std::move(interface_name));
  auto func_parameters = generate_params_for_invoke_method(interface);

  auto invoke_fun_v = VertexAdaptor<op_function>::create(func_parameters, VertexAdaptor<op_seq>::create());
  LambdaGenerator::make_invoke_method(interface, ClassData::NAME_OF_INVOKE_METHOD, invoke_fun_v);
  add_assumptions(invoke_fun_v->func_id);
  // body of invoke_fun will be replaced, we need it only to overcome PreprocessFunctionPass
  // it helps when a lambda to this interface is passed nowhere
  invoke_fun_v->cmd_ref() = VertexAdaptor<op_seq>::create(VertexAdaptor<op_return>::create());

  auto registered_interface = G->try_register_class(interface);
  if (registered_interface != interface) {
    // somebody had registered interface while we were generating a new one
    invoke_fun_v->func_id.clear();
    interface.clear();
  } else {
    // for stable generation of comments
    invoke_fun_v->func_id->file_id = SrcFilePtr(new SrcFile());
    invoke_fun_v->func_id->file_id->unified_file_name = interface->name;
    G->register_function(invoke_fun_v->func_id);
  }
  return registered_interface;
}

std::string LambdaInterfaceGenerator::generate_name() const noexcept {
  auto assum_to_string = [](std::string lhs, const auto &assum) {
    return std::move(lhs) + type_out(assum->get_type_data(), gen_out_style::txt);
  };
  auto params_as_string = std::accumulate(param_assumptions_.begin(), param_assumptions_.end(), std::string{}, assum_to_string);
  auto interface_name = assum_to_string(std::move(params_as_string), return_assumption_);
  replace_non_alphanum_inplace(interface_name);

  return LambdaClassData::get_lambda_namespace() + "\\" + interface_name;
}

InterfacePtr LambdaInterfaceGenerator::generate_interface_class(std::string name) const noexcept {
  InterfacePtr interface{new ClassData{}};
  interface->class_type = ClassType::interface;
  interface->modifiers.set_abstract();
  interface->set_name_and_src_name(name, "");
  interface->file_id = stage::get_file();

  return interface;
}

VertexAdaptor<op_func_param_list> LambdaInterfaceGenerator::generate_params_for_invoke_method(InterfacePtr interface) const noexcept {
  std::vector<VertexAdaptor<op_func_param>> func_parameters;
  func_parameters.reserve(1 + param_assumptions_.size());
  interface->patch_func_add_this(func_parameters, {});
  for (size_t i = 0; i < param_assumptions_.size(); ++i) {
    auto var = VertexAdaptor<op_var>::create();
    var->str_val = "arg" + std::to_string(i);

    func_parameters.emplace_back(VertexAdaptor<op_func_param>::create(var));
  }

  return VertexAdaptor<op_func_param_list>::create(func_parameters);
}

void LambdaInterfaceGenerator::add_assumptions(FunctionPtr invoke_method) noexcept {
  auto params = invoke_method->get_params();
  for (size_t i = 1; i < params.size(); ++i) {
    invoke_method->assumptions_for_vars.emplace_back(params[i].as<op_func_param>()->var()->str_val, std::move(param_assumptions_[i - 1]));
  }
  invoke_method->assumption_for_return = std::move(return_assumption_);
  invoke_method->assumption_return_status = AssumptionStatus::initialized;
  invoke_method->assumption_args_status = AssumptionStatus::initialized;
}

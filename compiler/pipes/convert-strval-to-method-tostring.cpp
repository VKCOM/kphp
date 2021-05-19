// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-strval-to-method-tostring.h"
#include "compiler/data/class-data.h"
#include "compiler/inferring/public.h"

// The function will try to convert the variable to a call to the
// __toString method if the variable has a class type and the class
// has such a method.
// If the conversion fails, an empty VertexPtr will be returned.
VertexPtr ConvertStrValToMagicMethodToStringPass::try_convert_var_to_call_to_string_method(VertexAdaptor<op_var> var) {
  const auto *type = tinf::get_type(var);
  if (type == nullptr) {
    return {};
  }

  auto klass = type->class_type();
  if (!klass) {
    return {};
  }

  if (!klass->has_to_string) {
    return {};
  }

  const auto *to_string_method = klass->get_instance_method(ClassData::NAME_OF_TO_STRING);
  if (to_string_method == nullptr) {
    return {};
  }

  auto args = std::vector<VertexAdaptor<op_var>>{var};
  auto call_function = VertexAdaptor<op_func_call>::create(args);
  call_function->set_string(std::string{to_string_method->local_name()});
  call_function->func_id = to_string_method->function;

  return call_function;
}

VertexPtr ConvertStrValToMagicMethodToStringPass::process_convert(VertexAdaptor<op_conv_string> conv) {
  auto expr = conv->expr();
  auto var = expr.try_as<op_var>();
  if (!var) {
    return conv;
  }

  auto call = try_convert_var_to_call_to_string_method(var);
  if (!call) {
    return conv;
  }

  return call;
}

void ConvertStrValToMagicMethodToStringPass::handle_print_r_call(VertexAdaptor<op_func_call> &call) {
  auto args = call->args();
  for (auto &arg : args) {
    auto var = arg.try_as<op_var>();
    if (!var) {
      continue;
    }

    auto call_func = try_convert_var_to_call_to_string_method(var);
    if (!call_func) {
      continue;
    }

    arg = call_func;
  }
}

VertexPtr ConvertStrValToMagicMethodToStringPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_func_call) {
    auto call = root.as<op_func_call>();
    if (call->func_id->name == "print_r") {
      handle_print_r_call(call);
    }
  }

  if (root->type() == op_conv_string) {
    return process_convert(root.as<op_conv_string>());
  }

  return root;
}

// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/convert-strval-to-method-tostring.h"
#include "compiler/data/class-data.h"
#include "compiler/inferring/public.h"

VertexPtr ConvertStrValToMagicMethodToStringPass::process_convert(VertexAdaptor<op_conv_string> conv) {
  auto expr = conv->expr();
  auto var = expr.try_as<op_var>();
  if (!var) {
    return conv;
  }

  auto call = convert_var_to_call_to_string_method(var);
  if (!call) {
    return conv;
  }

  return call;
}
VertexPtr ConvertStrValToMagicMethodToStringPass::convert_var_to_call_to_string_method(VertexAdaptor<op_var> var) {
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

VertexPtr ConvertStrValToMagicMethodToStringPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_conv_string) {
    return process_convert(root.as<op_conv_string>());
  }

  return root;
}

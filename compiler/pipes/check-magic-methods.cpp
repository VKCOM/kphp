// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-magic-methods.h"
#include "compiler/data/class-data.h"
#include "compiler/inferring/public.h"

VertexPtr CheckMagicMethodsPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_function) {
    return process_func(root.as<op_function>());
  }

  return root;
}

void CheckMagicMethodsPass::check_to_string(VertexAdaptor<op_function> func) {
  auto fun = func->func_id;
  stage::set_function(fun);

  auto count_args = fun->param_ids.size();
  kphp_error(count_args == 1, fmt_format("Magic method {} cannot take arguments", fun->get_human_readable_name()));

  const auto *ret_type = tinf::get_type(fun, -1);
  if (!ret_type) {
    return;
  }

  kphp_error(ret_type->ptype() == tp_string, fmt_format("Magic method {} must have string return type", fun->get_human_readable_name()));
}

VertexPtr CheckMagicMethodsPass::process_func(VertexAdaptor<op_function> func) {
  auto local_name = func->func_id->local_name();

  if (local_name == ClassData::NAME_OF_TO_STRING) {
    check_to_string(func);
  }

  return func;
}

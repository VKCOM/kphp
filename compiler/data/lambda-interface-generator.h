// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "common/smart_ptrs/intrusive_ptr.h"

#include "compiler/class-assumptions.h"
#include "compiler/data/data_ptr.h"

class LambdaInterfaceGenerator {
public:
  LambdaInterfaceGenerator(VertexAdaptor<op_type_expr_callable> type_expr) noexcept
    : type_expr(type_expr) {
  }

  InterfacePtr generate() noexcept;

private:
  std::string generate_name() const noexcept;
  InterfacePtr generate_interface_class(std::string name) const noexcept;
  VertexAdaptor<op_func_param_list> generate_params_for_invoke_method(InterfacePtr interface) const noexcept;
  void add_type_hints(FunctionPtr invoke_method) noexcept;

private:
  VertexAdaptor<op_type_expr_callable> type_expr;
};

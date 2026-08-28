// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {

std::pair<std::string, std::string> get_full_type_expr_str(vk::tlo_parsing::expr_base* type_expr, const std::string& var_num_access);

inline std::string get_full_value(vk::tlo_parsing::expr_base* type_expr, const std::string& var_num_access) {
  auto ans = get_full_type_expr_str(type_expr, var_num_access);
  return ans.second;
}

inline std::string get_full_type(vk::tlo_parsing::expr_base* type_expr, const std::string& var_num_access) {
  auto ans = get_full_type_expr_str(type_expr, var_num_access);
  return ans.first;
}

// Structure for any type expression store generation
struct TypeExprStore {
  const std::unique_ptr<vk::tlo_parsing::arg>& arg;
  std::string var_num_access;
  bool typed_mode;

  TypeExprStore(const std::unique_ptr<vk::tlo_parsing::arg>& arg, std::string var_num_access, bool typed_mode = false)
      : arg(arg),
        var_num_access(std::move(var_num_access)),
        typed_mode(typed_mode) {}

  void compile(CodeGenerator& W) const;
};

// Structure for any type expression fetch generation
struct TypeExprFetch {
  const std::unique_ptr<vk::tlo_parsing::arg>& arg;
  std::string var_num_access;
  bool typed_mode;

  explicit inline TypeExprFetch(const std::unique_ptr<vk::tlo_parsing::arg>& arg, std::string var_num_access, bool typed_mode = false)
      : arg(arg),
        var_num_access(std::move(var_num_access)),
        typed_mode(typed_mode) {}

  void compile(CodeGenerator& W) const;
};
} // namespace tl2cpp

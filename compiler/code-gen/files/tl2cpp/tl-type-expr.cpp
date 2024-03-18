// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tl2cpp/tl-type-expr.h"

#include "common/tl/constants/common.h"

namespace tl2cpp {
// Recursively traverse the type expression tree and return <type, value> pair
std::pair<std::string, std::string> get_full_type_expr_str(vk::tlo_parsing::expr_base *type_expr, const std::string &var_num_access) {
  if (auto *as_nat_var = type_expr->as<vk::tlo_parsing::nat_var>()) {
    return {"", var_num_access + cur_combinator->get_var_num_arg(as_nat_var->var_num)->name};
  }
  if (auto *as_nat_const = type_expr->as<vk::tlo_parsing::nat_const>()) {
    return {"", std::to_string(as_nat_const->num)};
  }
  if (auto *as_type_var = type_expr->as<vk::tlo_parsing::type_var>()) {
    std::string expr = "std::move(" + cur_combinator->get_var_num_arg(as_type_var->var_num)->name + ")";
    if (cur_combinator->is_constructor()) {
      return {"T" + std::to_string(as_type_var->var_num), expr};
    } else {
      return {"tl_exclamation_fetch_wrapper", expr};
    }
  }
  if (auto *as_type_array = type_expr->as<vk::tlo_parsing::type_array>()) {
    std::string inner_magic = "0";
    // after the replace_anonymous_args TL array that had multiple items now has only one item
    // that became a named type that contains all data from the original item;
    // size*[A B] -> size*[T] where T has A and B as its fields
    kphp_assert(as_type_array->args.size() == 1);
    if (auto *casted = as_type_array->args[0]->type_expr->as<vk::tlo_parsing::type_expr>()) {
      if (is_magic_processing_needed(casted)) {
        inner_magic = fmt_format("{:#010x}", static_cast<unsigned int>(type_of(casted)->id));
      }
    } else {
      kphp_error(false, "Too complicated tl array");
    }
    std::string array_item_type_name = cpp_tl_struct_name("t_", type_of(as_type_array->args[0]->type_expr)->name);
    std::string type = fmt_format("tl_array<{}, {}>", array_item_type_name, inner_magic);
    return {type, type + fmt_format("({}, {}())", get_full_value(as_type_array->multiplicity.get(), var_num_access), array_item_type_name)};
  }
  auto *as_type_expr = type_expr->as<vk::tlo_parsing::type_expr>();
  kphp_assert(as_type_expr);
  const auto &type = tl->types[as_type_expr->type_id];
  std::vector<std::string> template_params;
  std::vector<std::string> constructor_params;
  for (const auto &child : as_type_expr->children) {
    auto child_type_str = get_full_type_expr_str(child.get(), var_num_access);
    std::string child_type_name = child_type_str.first;
    std::string child_type_value = child_type_str.second;
    constructor_params.emplace_back(child_type_value);
    if (!child_type_name.empty()) {
      kphp_assert(child->as<vk::tlo_parsing::type_expr_base>());
      template_params.emplace_back(child_type_name);
      if (auto *child_as_type_expr = child->as<vk::tlo_parsing::type_expr>()) {
        if (is_magic_processing_needed(child_as_type_expr)) {
          template_params.emplace_back(fmt_format("{:#010x}", static_cast<unsigned int>(type_of(child_as_type_expr)->id)));
        } else {
          template_params.emplace_back("0");
        }
      } else if (auto *child_as_type_var = child->as<vk::tlo_parsing::type_var>()) {
        if (vk::string_view(child_type_name).starts_with("T")) {
          template_params.emplace_back(fmt_format("inner_magic{}", child_as_type_var->var_num));
        } else {
          template_params.emplace_back("0");
        }
      } else {
        kphp_assert(child->as<vk::tlo_parsing::type_array>());
        template_params.emplace_back("0");
      }
    }
  }
  auto type_name = (!type->is_integer_variable() ? cpp_tl_struct_name("t_", type->name) : cpp_tl_struct_name("t_", tl->types[TL_INT]->name));
  auto template_str = (!template_params.empty() ? "<" + vk::join(template_params, ", ") + ">" : "");
  auto full_type_name = type_name + template_str;
  auto full_type_value = full_type_name + "(" + vk::join(constructor_params, ", ") + ")";
  return {full_type_name, full_type_value};
}

void TypeExprStore::compile(CodeGenerator &W) const {
  std::string magic_storing = get_magic_storing(arg->type_expr.get());
  if (!magic_storing.empty()) {
    W << magic_storing << NL;
  }
  std::string target_expr = get_full_value(arg->type_expr.get(), var_num_access);
  if (!typed_mode) {
    W << target_expr << fmt_format(".store(tl_arr_get(tl_object, {}, {}, {}L))", register_tl_const_str(arg->name), arg->idx, hash_tl_const_str(arg->name));
  } else {
    W << target_expr << fmt_format(".typed_store({})", get_tl_object_field_access(arg, field_rw_type::READ));
  }
  W << ";" << NL;
}

void TypeExprFetch::compile(CodeGenerator &W) const {
  const auto magic_fetching =
    get_magic_fetching(arg->type_expr.get(), fmt_format("Incorrect magic of arg: {}\\nin constructor: {}", arg->name, cur_combinator->name));
  if (!magic_fetching.empty()) {
    W << magic_fetching << NL;
  }
  if (!typed_mode) {
    W << "result.set_value(" << register_tl_const_str(arg->name) << ", " << get_full_value(arg->type_expr.get(), var_num_access) << ".fetch(), "
      << hash_tl_const_str(arg->name) << "L);" << NL;
  } else {
    W << get_full_value(arg->type_expr.get(), var_num_access) + ".typed_fetch_to(" << get_tl_object_field_access(arg, field_rw_type::WRITE) << ");" << NL;
  }
}
} // namespace tl2cpp

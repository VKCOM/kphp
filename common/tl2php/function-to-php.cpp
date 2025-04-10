// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/function-to-php.h"

#include "common/tl2php/tl-to-php-classes-converter.h"

namespace vk {
namespace tl {

FunctionToPhp::FunctionToPhp(TlToPhpClassesConverter& tl_to_php, const tlo_parsing::combinator& tl_function)
    : CombinatorToPhp(tl_to_php, tl_function),
      exclamation_interface_(std::cref(tl_to_php.get_rpc_function_interface())) {}

PhpClassField FunctionToPhp::return_type_to_php_field() {
  assert(tl_combinator_.result);
  assert(php_doc_.empty());
  assert(type_stack_.empty());

  has_class_inside_ = false;
  last_processed_type_ = php_field_type::t_unknown;
  const auto old_exclamation_type = update_exclamation_interface(tl_to_php_.get_rpc_function_return_result_interface());
  tl_combinator_.result->visit(*this);
  update_exclamation_interface(old_exclamation_type);

  assert(type_stack_.empty());
  assert(last_processed_type_ != php_field_type::t_unknown);
  PhpClassField return_type{"value", std::move(php_doc_), "", 0, last_processed_type_, has_class_inside_};
  return return_type;
}

std::unique_ptr<CombinatorToPhp> FunctionToPhp::clone(const std::vector<php_field_type>& type_stack) const {
  auto result = std::make_unique<FunctionToPhp>(tl_to_php_, tl_combinator_);
  result->type_stack_ = type_stack;
  return std::move(result);
}

const PhpClassRepresentation& FunctionToPhp::update_exclamation_interface(const PhpClassRepresentation& interface) {
  auto old = exclamation_interface_;
  exclamation_interface_ = std::cref(interface);
  return old;
}

void FunctionToPhp::apply(const tlo_parsing::type_var& tl_type_var) {
  for (const auto& combinator_arg : tl_combinator_.args) {
    if (combinator_arg->is_forwarded_function()) {
      auto* excl_type_var = combinator_arg->type_expr->as<tlo_parsing::type_var>();
      assert(excl_type_var);
      if (excl_type_var->var_num == tl_type_var.var_num) {
        last_processed_type_ = php_field_type::t_class;
        has_class_inside_ = true;
        if (!is_template_instantiating()) {
          php_doc_.append(exclamation_interface_.get().php_class_namespace);
          php_doc_.append("\\");
        }
        php_doc_.append(exclamation_interface_.get().php_class_name);
        return;
      }
    }
  }
  auto* arg_ptr = tl_combinator_.get_var_num_arg(tl_type_var.var_num);
  assert(arg_ptr);
  arg_ptr->type_expr->visit(*this);
}

} // namespace tl
} // namespace vk

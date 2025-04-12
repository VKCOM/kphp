// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/constructor-to-php.h"

#include "common/tl2php/tl-to-php-classes-converter.h"

namespace vk {
namespace tl {

ConstructorToPhp::ConstructorToPhp(TlToPhpClassesConverter &tl_to_php,
                                   const tlo_parsing::combinator &tl_constructor,
                                   CombinatorToPhp &outer_converter,
                                   const tlo_parsing::type_expr &outer_type_expr) :
  CombinatorToPhp(tl_to_php, tl_constructor),
  outer_converter_(outer_converter),
  outer_type_expr_(outer_type_expr) {
}

std::unique_ptr<CombinatorToPhp> ConstructorToPhp::clone(const std::vector<php_field_type> &type_stack) const {
  auto result = std::make_unique<ConstructorToPhp>(tl_to_php_, tl_combinator_, outer_converter_, outer_type_expr_);
  result->type_stack_ = type_stack;
  return std::move(result);
}

const PhpClassRepresentation &ConstructorToPhp::update_exclamation_interface(const PhpClassRepresentation &interface) {
  assert(&interface == &tl_to_php_.get_rpc_function_return_result_interface());
  return interface;
}

void ConstructorToPhp::apply(const tlo_parsing::type_var &tl_type_var) {
  const size_t template_arg_number = tl_combinator_.get_type_parameter_input_index(tl_type_var.var_num);
  assert(template_arg_number < outer_type_expr_.children.size());
  auto outer_converter_clone = outer_converter_.clone(type_stack_);
  outer_converter_clone->update_exclamation_interface(tl_to_php_.get_rpc_function_return_result_interface());
  outer_type_expr_.children[template_arg_number]->visit(*outer_converter_clone);
  apply_state(*outer_converter_clone);
}

} // namespace tl
} // namespace vk

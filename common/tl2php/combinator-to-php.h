// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/tl2php/php-classes.h"
#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {

class TlToPhpClassesConverter;

class CombinatorToPhp : protected tlo_parsing::const_expr_visitor {
public:
  CombinatorToPhp(TlToPhpClassesConverter &tl_to_php, const tlo_parsing::combinator &tl_combinator);

  std::vector<PhpClassField> args_to_php_class_fields(const std::vector<std::unique_ptr<tlo_parsing::arg>> &args);

  void apply_state(const CombinatorToPhp &other);

  virtual std::unique_ptr<CombinatorToPhp> clone(const std::vector<php_field_type> &type_stack) const = 0;
  virtual const PhpClassRepresentation &update_exclamation_interface(const PhpClassRepresentation &interface) = 0;

protected:
  friend class AutoPushPop;

  PhpClassField arg_type_to_php_field(const tlo_parsing::arg &combinator_arg);

  bool try_as_primitive_builtin_type(const vk::string_view &tl_type_name);
  bool try_as_array_type(const tlo_parsing::type_expr &tl_type_expr, const vk::string_view &tl_type_name);
  void as_array_type(const tlo_parsing::expr_base &array_item);
  bool try_as_maybe_type(const tlo_parsing::type_expr &tl_type_expr, const vk::string_view &tl_type_name);
  void as_complex_type(const tlo_parsing::type_expr &tl_type_expr, const tlo_parsing::type &tl_type);

  void apply(const tlo_parsing::type_expr &tl_type_expr) final;
  void apply(const tlo_parsing::type_array &tl_type_array) final;
  void apply(const tlo_parsing::nat_const &) final {}
  void apply(const tlo_parsing::nat_var &) final {}

  bool is_template_instantiating() const;
  bool is_array_instantiating() const;

  std::string php_doc_;
  php_field_type last_processed_type_{php_field_type::t_unknown};

  std::vector<php_field_type> type_stack_;

  TlToPhpClassesConverter &tl_to_php_;
  const tlo_parsing::combinator &tl_combinator_;
  bool has_class_inside_{false};
};
} // namespace tl
} // namespace vk

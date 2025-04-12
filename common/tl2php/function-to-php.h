// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/tl2php/combinator-to-php.h"

namespace vk {
namespace tl {

class FunctionToPhp final : public CombinatorToPhp {
public:
  FunctionToPhp(TlToPhpClassesConverter& tl_to_php, const tlo_parsing::combinator& tl_function);

  PhpClassField return_type_to_php_field();

private:
  std::unique_ptr<CombinatorToPhp> clone(const std::vector<php_field_type>& type_stack) const final;
  const PhpClassRepresentation& update_exclamation_interface(const PhpClassRepresentation& interface) final;

  using CombinatorToPhp::apply;
  void apply(const tlo_parsing::type_var& tl_type_var) final;

  std::reference_wrapper<const PhpClassRepresentation> exclamation_interface_;
};

} // namespace tl
} // namespace vk

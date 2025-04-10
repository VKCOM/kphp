// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/php-classes.h"

#include "common/algorithms/find.h"
#include "common/tl2php/tl-to-php-classes-converter.h"
#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {

void PhpClasses::load_from(const vk::tlo_parsing::tl_scheme& scheme, bool generate_tl_internals) {
  TlToPhpClassesConverter tl_to_php_doc_converter{scheme, *this};
  for (const auto& func : scheme.functions) {
    const tlo_parsing::combinator& tl_combinator = *func.second;
    if (generate_tl_internals || !tl_combinator.is_internal_function()) {
      tl_to_php_doc_converter.register_function(tl_combinator);
    }
  }
}

bool is_or_null_possible(php_field_type internal_type) {
  return vk::none_of_equal(internal_type, php_field_type::t_mixed, php_field_type::t_maybe);
}

bool is_php_code_gen_allowed(const TlTypePhpRepresentation& type_repr) noexcept {
  // tl type RpcReqResult has multiple constructors, but one named as itself, it's incorrect, skip it
  return type_repr.type_representation->tl_name != "RpcReqResult";
}

bool is_php_code_gen_allowed(const TlFunctionPhpRepresentation& func_repr) noexcept {
  // this tl function uses 'RpcReqResult' and should never be used from php code
  return func_repr.function_args->tl_name != "rpcInvokeReq";
}

std::string PhpClassRepresentation::get_full_php_class_name() const {
  return "VK\\" + php_class_namespace + "\\" + php_class_name;
}

} // namespace tl
} // namespace vk

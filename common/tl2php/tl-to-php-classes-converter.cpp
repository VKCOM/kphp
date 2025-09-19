// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/tl-to-php-classes-converter.h"

#include <utility>

#include "common/algorithms/string-algorithms.h"
#include "common/tl2php/constructor-to-php.h"
#include "common/tl2php/function-to-php.h"
#include "common/tl2php/php-classes.h"
#include "common/tlo-parsing/tl-objects.h"

namespace vk {
namespace tl {

struct PhpName {
  PhpName(const std::string& name, const std::string& inner_namespace = {}, const std::string& postfix = {})
      : tl_name(name) {
    assert(postfix.find('.') == std::string::npos);
    class_namespace = PhpClasses::tl_namespace();

    const auto dot_pos = name.find('.');
    if (dot_pos == std::string::npos) {
      class_short_name = name + postfix;
      if (!inner_namespace.empty()) {
        class_namespace.append("\\").append(PhpClasses::common_engine_namespace()).append("\\").append(inner_namespace);
      }
      class_full_name = class_namespace + '\\' + class_short_name;
      return;
    }

    assert(name.find('.', dot_pos + 1) == std::string::npos);
    assert(dot_pos != 0 && dot_pos + 1 != name.size());
    class_short_name.assign(name).append(postfix);
    class_short_name[dot_pos] = '_';
    class_namespace.append("\\").append(name, 0, dot_pos);
    if (!inner_namespace.empty()) {
      class_namespace.append("\\").append(inner_namespace);
    }
    class_full_name = class_namespace + '\\' + class_short_name;
  }

  PhpName& append_name(const char* postfix) {
    class_full_name.append(postfix);
    class_short_name.append(postfix);
    return *this;
  }

  std::string tl_name;
  std::string class_full_name;
  std::string class_short_name;
  std::string class_namespace;
};

TlToPhpClassesConverter::TlToPhpClassesConverter(const tlo_parsing::tl_scheme& scheme, PhpClasses& load_into)
    : php_classes_(load_into),
      scheme_(scheme) {
  php_classes_.functions.reserve(scheme_.functions.size());
  php_classes_.types.reserve(scheme_.types.size());
  php_classes_.all_classes.reserve(scheme_.functions.size() + scheme_.types.size());

  register_builtin_classes();
}

void TlToPhpClassesConverter::register_function(const tlo_parsing::combinator& tl_function) {
  PhpName function_name{tl_function.name, PhpClasses::functions_namespace()};
  FunctionToPhp function_to_php_doc{*this, tl_function};
  auto& tl_function_php_repr = php_classes_.functions[function_name.class_full_name];
  assert(!tl_function_php_repr.function_args);

  tl_function_php_repr.function_args =
      make_and_register_new_class(function_name, function_to_php_doc.args_to_php_class_fields(tl_function.args), tl_function.id, rpc_function_interface_);
  tl_function_php_repr.function_result = make_and_register_new_class(function_name.append_name("_result"), {function_to_php_doc.return_type_to_php_field()},
                                                                     tl_function.id, rpc_function_return_result_interface_);
  tl_function_php_repr.is_kphp_rpc_server_function = tl_function.is_kphp_rpc_server_function();
}

void TlToPhpClassesConverter::register_type(const tlo_parsing::type& tl_type, std::string type_postfix, CombinatorToPhp& outer_converter,
                                            const tlo_parsing::type_expr& outer_type_expr, bool is_builtin) {
  PhpName type_name{tl_type.name, is_builtin ? "" : PhpClasses::types_namespace(), type_postfix};
  auto types_it = php_classes_.types.find(type_name.class_full_name);
  auto known_type_it = known_type_.emplace(type_name.class_full_name, std::cref(tl_type));
  if (types_it != php_classes_.types.end()) {
    assert(!known_type_it.second);
    assert(&known_type_it.first->second.get() == &tl_type);
    return;
  }

  auto& tl_type_php_repr = php_classes_.types[type_name.class_full_name];
  assert(!tl_type.constructors.empty());
  if (tl_type.is_polymorphic()) {
    tl_type_php_repr.type_representation = make_and_register_new_interface(type_name, tl_type.id, is_builtin);
    tl_type_php_repr.constructors.reserve(tl_type.constructors.size());
    for (const auto& tl_constructor : tl_type.constructors) {
      PhpName constructor_name{tl_constructor->name, PhpClasses::types_namespace(), type_postfix};
      ConstructorToPhp constructor_to_php_doc{*this, *tl_constructor, outer_converter, outer_type_expr};
      tl_type_php_repr.constructors.emplace_back(make_and_register_new_class(constructor_name,
                                                                             constructor_to_php_doc.args_to_php_class_fields(tl_constructor->args),
                                                                             tl_constructor->id, tl_type_php_repr.type_representation.get()));
    }
  } else {
    const auto& tl_constructor = tl_type.constructors.front();
    PhpName constructor_name{tl_constructor->name, PhpClasses::types_namespace(), type_postfix};
    ConstructorToPhp constructor_to_php_doc{*this, *tl_constructor, outer_converter, outer_type_expr};
    tl_type_php_repr.type_representation =
        make_and_register_new_class(constructor_name, constructor_to_php_doc.args_to_php_class_fields(tl_constructor->args), tl_constructor->id);
  }
}

const tlo_parsing::tl_scheme& TlToPhpClassesConverter::get_sheme() const {
  return scheme_;
}

const PhpClassRepresentation& TlToPhpClassesConverter::get_rpc_function_interface() const {
  return *rpc_function_interface_;
}

const PhpClassRepresentation& TlToPhpClassesConverter::get_rpc_function_return_result_interface() const {
  return *rpc_function_return_result_interface_;
}

bool TlToPhpClassesConverter::need_rpc_response_type_hack(vk::string_view php_doc) {
  if (php_doc.ends_with(rpc_function_return_result_interface_->php_class_name)) {
    php_doc = php_doc.substr(0, php_doc.size() - rpc_function_return_result_interface_->php_class_name.size());
    if (php_doc.ends_with("__")) {
      return php_doc.substr(0, php_doc.size() - 2).ends_with(PhpClasses::rpc_response_type());
    }
  }
  return false;
}

void TlToPhpClassesConverter::register_builtin_classes() {
  PhpName function_interface_name{PhpClasses::rpc_function()};
  auto& function_repr = php_classes_.functions[function_interface_name.class_full_name];
  function_repr.function_args = make_and_register_new_interface(function_interface_name, 0, true);
  rpc_function_interface_ = function_repr.function_args.get();

  PhpName function_return_result_interface_name{PhpClasses::rpc_function_return_result()};
  function_repr.function_result = make_and_register_new_interface(function_return_result_interface_name, 0, true);
  rpc_function_return_result_interface_ = function_repr.function_result.get();
}

std::unique_ptr<PhpClassRepresentation> TlToPhpClassesConverter::make_and_register_new_class(const PhpName& name, std::vector<PhpClassField> fields,
                                                                                             int magic_id, const PhpClassRepresentation* parent) {
  auto php_repr = std::make_unique<PhpClassRepresentation>();
  php_repr->php_class_namespace = name.class_namespace;
  php_repr->php_class_name = name.class_short_name;
  assert(!parent || parent->is_interface);
  php_repr->parent = parent;
  php_repr->class_fields = std::move(fields);
  php_repr->is_interface = false;
  php_repr->tl_name = name.tl_name;
  php_repr->magic_id = magic_id;
  const auto emplaced = php_classes_.all_classes.emplace(name.class_full_name, *php_repr).second;
  assert(emplaced);
  php_classes_.magic_to_classes.emplace(magic_id, *php_repr);
  return php_repr;
}

std::unique_ptr<PhpClassRepresentation> TlToPhpClassesConverter::make_and_register_new_interface(const PhpName& name, int magic_id, bool is_builtin) {
  auto php_repr = make_and_register_new_class(name, {}, magic_id);
  php_repr->is_interface = true;
  php_repr->is_builtin = is_builtin;
  return php_repr;
}

} // namespace tl
} // namespace vk

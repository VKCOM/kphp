// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-tl-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/phpdoc.h"

namespace {

void verify_class_against_repr(ClassPtr class_id, const vk::tl::PhpClassRepresentation& repr) {
  kphp_error_return(repr.is_builtin == class_id->is_builtin(),
                    fmt_format("Tl-class '{}' is{} expected to be builtin", class_id->name, repr.is_builtin ? "" : "not"));
  kphp_error_return(class_id->is_interface() == repr.is_interface,
                    fmt_format("Tl-class '{}' is{} expected to be an interface", class_id->name, repr.is_interface ? "" : " not"));

  for (const auto& child : class_id->derived_classes) {
    kphp_error_return(child->phpdoc->has_tag(PhpDocType::kphp_tl_class),
                      fmt_format("Class '{}' is expected to be tl-class, because it inherits from tl-class '{}'", child->name, class_id->name));
  }

  if (repr.parent) {
    const std::string expected = repr.parent->php_class_namespace + "\\" + repr.parent->php_class_name;
    kphp_error_return(class_id->implements.size() == 1, fmt_format("Tl-class '{}' must inherit '{}'", class_id->name, expected));
    const vk::string_view got{class_id->implements.front()->name};
    kphp_error_return(got.ends_with(expected), fmt_format("Tl-class '{}' must inherit '{}', but it inherits '{}'", class_id->name, expected, got));
  }

  for (const auto& field : repr.class_fields) {
    const auto* member = class_id->members.find_by_local_name<ClassMemberInstanceField>(field.field_name);
    kphp_error_return(member, fmt_format("Can't find field '{}' in tl-class '{}'", field.field_name, class_id->name));
  }
}

void check_class(ClassPtr class_id) {
  if (class_id->is_tl_class) {
    const size_t pos = class_id->name.find(vk::tl::PhpClasses::tl_namespace());
    kphp_error_return(pos != std::string::npos, fmt_format("Bad tl-class '{}' namespace", class_id->name));
    std::string tl_class_name = class_id->name.substr(pos);
    const auto& tl_php_classes = G->get_tl_classes().get_php_classes();
    auto tl_class_php_repr_it = tl_php_classes.all_classes.find(tl_class_name);
    kphp_error_return(tl_class_php_repr_it != tl_php_classes.all_classes.end(), fmt_format("Can't find tl-class '{}' in schema", class_id->name));
    verify_class_against_repr(class_id, tl_class_php_repr_it->second.get());
  }
}

} // namespace

bool CheckTlClasses::check_function(FunctionPtr) const {
  // TODO it should be better
  return !G->settings().tl_schema_file.get().empty();
}

void CheckTlClasses::on_start() {
  if (current_function->type == FunctionData::func_class_holder) {
    check_class(current_function->class_id);
  }
}

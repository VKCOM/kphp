#include "compiler/pipes/check-tl-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/phpdoc.h"

namespace {

void verify_class_against_repr(ClassPtr class_id, const vk::tl::PhpClassRepresentation &repr) {
  kphp_error_return(repr.is_builtin == class_id->is_builtin(),
                    format("Tl-class '%s' is%s expected to be builtin", repr.is_builtin ? "" : "not", class_id->name.c_str()));
  kphp_error_return(class_id->is_interface() == repr.is_interface,
                    format("Tl-class '%s' is%s expected to be an interface", class_id->name.c_str(), repr.is_interface ? "" : " not"));

  for (const auto &child : class_id->derived_classes) {
    kphp_error_return(phpdoc_tag_exists(child->phpdoc_str, php_doc_tag::kphp_tl_class),
                      format("Class '%s' is expected to be tl-class, because it inherits from tl-class '%s'",
                             child->name.c_str(), class_id->name.c_str()));
  }

  if (repr.parent) {
    const std::string expected = repr.parent->php_class_namespace + "\\" + repr.parent->php_class_name;
    kphp_error_return(class_id->implements.size() == 1,
                      format("Tl-class '%s' must inherit '%s'", class_id->name.c_str(), expected.c_str()));
    const vk::string_view got{class_id->implements.front()->name};
    kphp_error_return(got.ends_with(expected),
                      fmt_format("Tl-class '{}' must inherit '{}', but it inherits '{}'",
                                 class_id->name, expected, got));
  }

  for (const auto &field : repr.class_fields) {
    auto member = class_id->members.find_by_local_name<ClassMemberInstanceField>(field.field_name);
    kphp_error_return(member, format("Can't find field '%s' in tl-class '%s'",
                                     field.field_name.c_str(), class_id->name.c_str()));
    kphp_error_return(member->phpdoc_str.find(field.php_doc_type) != std::string::npos,
                      format("Field '%s' of tl-class '%s' has incorrect php doc", field.field_name.c_str(), class_id->name.c_str()));
  }
}

void check_class(ClassPtr class_id) {
  if (class_id->is_tl_class) {
    const size_t pos = class_id->name.find(vk::tl::PhpClasses::tl_namespace());
    kphp_error_return(pos != std::string::npos,
                      format("Bad tl-class '%s' namespace", class_id->name.c_str()));
    std::string tl_class_name = class_id->name.substr(pos);
    const auto &tl_php_classes = G->get_tl_classes().get_php_classes();
    auto tl_class_php_repr_it = tl_php_classes.all_classes.find(tl_class_name);
    kphp_error_return(tl_class_php_repr_it != tl_php_classes.all_classes.end(),
                      format("Can't find tl-class '%s' in schema", class_id->name.c_str()));
    verify_class_against_repr(class_id, tl_class_php_repr_it->second.get());
  }
}

}

bool CheckTlClasses::check_function(FunctionPtr function) {
  if (G->env().get_tl_schema_file().empty()) {
    // TODO it should be better
    return false;
  }
  if (function->type == FunctionData::func_class_holder) {
    check_class(function->class_id);
  }
  return false;
}

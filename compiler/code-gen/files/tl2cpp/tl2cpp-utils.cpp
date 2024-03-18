// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

#include "common/tl/constants/common.h"

#include "common/php-functions.h"

namespace tl2cpp {
const std::string T_TYPE = "Type";
const std::unordered_set<std::string> CUSTOM_IMPL_TYPES{"#",
                                                        T_TYPE,
                                                        "Int",
                                                        "Long",
                                                        "Double",
                                                        "Float",
                                                        "String",
                                                        "Bool",
                                                        "True",
                                                        "False",
                                                        "Vector",
                                                        "Maybe",
                                                        "Tuple",
                                                        "Dictionary",
                                                        "DictionaryField",
                                                        "IntKeyDictionary",
                                                        "IntKeyDictionaryField",
                                                        "LongKeyDictionary",
                                                        "LongKeyDictionaryField"};

static const std::string VK_name_prefix = "VK\\";
vk::tlo_parsing::tl_scheme *tl;
const vk::tlo_parsing::combinator *cur_combinator;

std::set<std::string> tl_const_vars;

std::string cpp_tl_const_str(std::string tl_name) {
  std::replace(tl_name.begin(), tl_name.end(), '.', '$');
  return "tl_str$" + tl_name;
}

std::string register_tl_const_str(const std::string &tl_name) {
  tl_const_vars.insert(tl_name);
  return cpp_tl_const_str(tl_name);
}

int64_t hash_tl_const_str(const std::string &tl_name) {
  return string_hash(tl_name.c_str(), tl_name.size());
}

std::string cpp_tl_struct_name(const char *prefix, std::string tl_name, const std::string &template_args_postfix) {
  std::replace(tl_name.begin(), tl_name.end(), '.', '_');
  return std::string(prefix) + tl_name + template_args_postfix;
}

std::string get_magic_storing(const vk::tlo_parsing::type_expr_base *arg_type_expr) {
  if (const auto *arg_as_type_expr = arg_type_expr->template as<vk::tlo_parsing::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      return fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(type_of(arg_as_type_expr)->id));
    }
  } else if (const auto *arg_as_type_var = arg_type_expr->template as<vk::tlo_parsing::type_var>()) {
    return fmt_format("store_magic_if_not_bare(inner_magic{});", arg_as_type_var->var_num);
  }
  return "";
}

std::string get_magic_fetching(const vk::tlo_parsing::type_expr_base *arg_type_expr, const std::string &error_msg) {
  if (const auto *arg_as_type_expr = arg_type_expr->template as<vk::tlo_parsing::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      return fmt_format(R"(fetch_magic_if_not_bare({:#010x}, "{}");)", static_cast<unsigned int>(type_of(arg_as_type_expr)->id), error_msg);
    }
  } else if (const auto *arg_as_type_var = arg_type_expr->template as<vk::tlo_parsing::type_var>()) {
    for (const auto &arg : cur_combinator->args) {
      if (auto *casted = arg->type_expr->as<vk::tlo_parsing::type_var>()) {
        if (arg->is_forwarded_function() && casted->var_num == arg_as_type_var->var_num) {
          return "";
        }
      }
    }
    return fmt_format(R"(fetch_magic_if_not_bare(inner_magic{}, "{}");)", arg_as_type_var->var_num, error_msg);
  }
  return "";
}

// The renamed TL scheme is needed inside every function that try to get a PHP object using the TL object (or vice versa).
// Some examples of such functions:
// 1) TlTemplateTypeHelpers
// 2) get_php_runtime_type(...)
// 3) get_php_class_of_tl_*(...)
vk::tlo_parsing::type *get_this_from_renamed_tl_scheme(const vk::tlo_parsing::type *t) {
  return G->get_tl_classes().get_scheme()->get_type_by_magic(t->id);
}

vk::tlo_parsing::combinator *get_this_from_renamed_tl_scheme(const vk::tlo_parsing::combinator *c) {
  return c->is_constructor() ? G->get_tl_classes().get_scheme()->get_constructor_by_magic(c->id)
                             : G->get_tl_classes().get_scheme()->get_function_by_magic(c->id);
}

std::string get_php_namespace(const std::string &tl_name) {
  size_t dot_pos = tl_name.find('.');
  return dot_pos == std::string::npos ? vk::tl::PhpClasses::common_engine_namespace() : tl_name.substr(0, dot_pos);
}

// find the TL constructor by the PHP-class
// for example, class VK\TL\messages\Types\messages_chatInfo => messages.chatInfo
// it could be a specialization of the dependent type constructor: Types\vectorTotal__int => vectorTotal
vk::tlo_parsing::combinator *get_tl_constructor_of_php_class(ClassPtr klass) {
  const auto &tl_scheme = G->get_tl_classes().get_scheme();

  const auto &php_representations = G->get_tl_classes().get_php_classes().all_classes;
  auto constructor_php_repr_iter = php_representations.find(klass->name.substr(VK_name_prefix.length()));
  kphp_assert(constructor_php_repr_iter != php_representations.end());

  const auto &constructor_magic_iter = tl_scheme->magics.find(constructor_php_repr_iter->second.get().tl_name);
  kphp_assert(constructor_magic_iter != tl_scheme->magics.end());

  vk::tlo_parsing::combinator *res = tl_scheme->get_constructor_by_magic(constructor_magic_iter->second);
  kphp_assert(res);
  return res;
}

// find the TL polymorphic type by the PHP-interface;
// it won't work for non-polymorphic types as we'll have constructors instead of types for them (e.g. class messages_chatInfo);
// therefore, this function works only for polymorphic types (e.g. interface memcache_Value)
vk::tlo_parsing::type *get_tl_type_of_php_class(ClassPtr interface) {
  kphp_assert(is_php_class_a_tl_polymorphic_type(interface));
  const auto &tl_scheme = G->get_tl_classes().get_scheme();

  const auto &tl_type_php_representations = G->get_tl_classes().get_php_classes().types;
  auto php_repr_iter = tl_type_php_representations.find(interface->name.substr(VK_name_prefix.length()));
  kphp_assert(php_repr_iter != tl_type_php_representations.end());

  auto tl_magic_iter = tl_scheme->magics.find(php_repr_iter->second.type_representation->tl_name);
  kphp_assert(tl_magic_iter != tl_scheme->magics.end());

  auto *res_tl_type = tl_scheme->get_type_by_magic(tl_magic_iter->second);
  kphp_assert(res_tl_type);
  return res_tl_type;
}

// for dependent type constructors, like vectorTotal {t:Type},
// there can be multiple PHP-specializations like vectorTotal__int, etc per one TL constructor.
// this function accepts the concrete specialization of such constructor with the raw suffix of the PHP class (e.g. "__int")
ClassPtr get_php_class_of_tl_constructor_specialization(const vk::tlo_parsing::combinator *c, const std::string &specialization_suffix) {
  auto *c_from_renamed = get_this_from_renamed_tl_scheme(c);
  std::string php_namespace = get_php_namespace(c_from_renamed->name);
  std::string php_class_name =
    G->settings().tl_namespace_prefix.get() + php_namespace + "\\Types\\" + replace_characters(c_from_renamed->name, '.', '_') + specialization_suffix;
  return G->get_class(php_class_name);
}

std::vector<ClassPtr> _get_all_php_classes_by_tl_magic(int magic, bool need_only_interfaces) {
  std::vector<ClassPtr> classes;
  auto range = G->get_tl_classes().get_php_classes().magic_to_classes.equal_range(magic);
  for (auto it = range.first; it != range.second; ++it) {
    const vk::tl::PhpClassRepresentation &php_representation = it->second.get();
    ClassPtr klass = G->get_class(php_representation.get_full_php_class_name());
    if (php_representation.is_interface == need_only_interfaces && klass) {
      classes.emplace_back(klass);
    }
  }
  return classes;
}

// For every TL constructor there can be one PHP-class (in case of the simple constructors like likes.item)
// or more if it's a constructor with dependent types (then we'll have vectorTotal__int and other specializations)
std::vector<ClassPtr> get_all_php_classes_of_tl_constructor(const vk::tlo_parsing::combinator *c) {
  auto *c_from_renamed = get_this_from_renamed_tl_scheme(c); // For the sake of consistency
  return _get_all_php_classes_by_tl_magic(c_from_renamed->id, false);
}

// For every TL type there can be one class/interface (in case of the simple types like likes.Item)
// or more if it's a type with dependent types (then we'll have Either_string_Vertex and other specializations)
std::vector<ClassPtr> get_all_php_classes_of_tl_type(const vk::tlo_parsing::type *t) {
  auto *t_from_renamed = get_this_from_renamed_tl_scheme(t); // For the sake of consistency
  return _get_all_php_classes_by_tl_magic(t_from_renamed->id, t_from_renamed->is_polymorphic());
}

// If type has dependent types, VectorTotal<t> for example, then one TL type will have
// many PHP-specializations vectorTotal__int, etc
// (non-polymorphic types are combined with constructors into a single class as usual)
ClassPtr get_php_class_of_tl_type_specialization(const vk::tlo_parsing::type *t, const std::string &specialization_suffix) {
  auto *t_from_renamed = get_this_from_renamed_tl_scheme(t);
  kphp_assert(is_type_dependent(t_from_renamed, G->get_tl_classes().get_scheme().get()));
  std::string php_namespace = get_php_namespace(t_from_renamed->name);
  std::string lookup_name = t_from_renamed->is_polymorphic() ? t_from_renamed->name : t_from_renamed->constructors[0]->name;
  std::string php_class_name =
    G->settings().tl_namespace_prefix.get() + php_namespace + "\\Types\\" + replace_characters(lookup_name, '.', '_') + specialization_suffix;
  return G->get_class(php_class_name);
}

// For the 'messages.getChatInfo' TL function there is a PHP class VK\TL\messages\Functions\messages_getChatInfo
// if that class is reachable by the compiler, typed form of the 'messages.getChatInfo' should be generated
// (otherwise, that typed TL function is guaranteed not to be called, hence we don't need it)
ClassPtr get_php_class_of_tl_function(const vk::tlo_parsing::combinator *f) {
  auto *f_from_renamed = get_this_from_renamed_tl_scheme(f);
  std::string php_namespace = get_php_namespace(f_from_renamed->name);
  std::string php_class_name = G->settings().tl_namespace_prefix.get() + php_namespace + "\\Functions\\" + replace_characters(f_from_renamed->name, '.', '_');
  return G->get_class(php_class_name);
}

// For the 'VK\TL\messages\Functions\messages_getChatInfo' PHP class there is a TL function 'messages.getChatInfo', etc
std::string get_tl_function_name_of_php_class(ClassPtr klass) {
  kphp_assert(is_php_class_a_tl_function(klass));
  size_t functions_ns_pos = vk::string_view{klass->name}.find("Functions\\");
  std::string after_ns_functions = static_cast<std::string>(vk::string_view{klass->name}.substr(functions_ns_pos + 10));
  size_t underscore_pos = klass->name.find('_');
  if (underscore_pos == std::string::npos) {
    return after_ns_functions;
  }
  // If name contains '_' then it should be replaced with '.';
  // there are at least two cases:
  // 1. '_' before '.' like in smart_alerts.sendMessage
  // 2. '_' after '.' like in expr.earth_distance
  std::string tmp = klass->name.substr(G->settings().tl_namespace_prefix.get().length());
  std::string php_namespace = tmp.substr(0, tmp.find('\\'));
  if (php_namespace == vk::tl::PhpClasses::common_engine_namespace()) {
    return after_ns_functions;
  }
  after_ns_functions.at(php_namespace.length()) = '.';
  return after_ns_functions;
}

// 'messages.getChatInfo' TL function has a typed result in form of the PHP class VK\TL\messages\Functions\messages_getChatInfo_result
// If it's reachable <=> the result is reachable
ClassPtr get_php_class_of_tl_function_result(const vk::tlo_parsing::combinator *f) {
  auto *f_from_renamed = get_this_from_renamed_tl_scheme(f);
  std::string php_namespace = get_php_namespace(f_from_renamed->name);
  std::string php_class_name =
    G->settings().tl_namespace_prefix.get() + php_namespace + "\\Functions\\" + replace_characters(f_from_renamed->name, '.', '_') + "_result";
  return G->get_class(php_class_name);
}

// classes VK\TL\*\Functions\* that implement RpcFunction are classes that correspond to the TL-functions
bool is_php_class_a_tl_function(ClassPtr klass) {
  return klass->is_tl_class && klass->implements.size() == 1 && vk::string_view{klass->implements.front()->name}.ends_with("RpcFunction");
}

// classes VK\TL\*\Types\* are a non-interface types that correspond to the TL-constructors
// (or non-polymorphic types with a single constructor that was inlined)
bool is_php_class_a_tl_constructor(ClassPtr klass) {
  return klass->is_tl_class && klass->is_class() && klass->name.find("Types\\") != std::string::npos;
}

// for every complex [array] types where array item is composed out of several members,
// we generate a separate PHP-class. For example:
// isearch.typeInfo n:# data:n*[type:int probability:double] = isearch.TypeInfo
// will produce a isearch\isearch_typeInfo_arg2_item { int type, double probability } class and we'll get:
// isearch.typeInfo n:# data:n*[isearch\isearch_typeInfo_arg2_item] = isearch.TypeInfo
bool is_php_class_a_tl_array_item(ClassPtr klass) {
  return klass->is_tl_class && klass->is_class() && klass->name.find("Types\\") != std::string::npos && klass->name.find("_arg") != std::string::npos
         && klass->name.find("_item") != std::string::npos;
}

bool is_tl_type_a_php_array(const vk::tlo_parsing::type *t) {
  return t->id == TL_VECTOR || t->id == TL_TUPLE || t->id == TL_DICTIONARY || t->id == TL_INT_KEY_DICTIONARY || t->id == TL_LONG_KEY_DICTIONARY;
}

bool is_tl_type_wrapped_to_Optional(const vk::tlo_parsing::type *type) {
  // [fields_mask.n? | Maybe] [int|string|array|double|bool] -- with Optional
  // [fields_mask.n? | Maybe] [class_instance<T>|Optional<T>|var] -- without Optional
  return is_tl_type_a_php_array(type) || vk::any_of_equal(type->id, TL_INT, TL_DOUBLE, TL_FLOAT, TL_STRING) || type->name == "Bool"
         || type->is_integer_variable() || type->id == TL_LONG;
}

// classes VK\TL\*\Types\* are a non-interface types that correspond to the TL-constructors
// (or non-polymorphic types with a single constructor that was inlined)

// classes VK\TL\Types\* are interfaces - polymorphic type with constructors that implement it
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass) {
  return klass->is_tl_class && klass->is_interface() && klass->name.find("Types\\") != std::string::npos;
}

bool is_magic_processing_needed(const vk::tlo_parsing::type_expr *type_expr) {
  const auto &type = tl->types[type_expr->type_id];
  // polymorphic types do the 'magic' processing internally; '#' never needs to process 'magic'
  bool handles_magic_inside = type->is_integer_variable() ? true : type->is_polymorphic();
  bool is_used_as_bare = type_expr->is_bare();
  // for polymorphic types and types with % we don't need to read 'magic' from the outside
  return !(handles_magic_inside || is_used_as_bare);
}

vk::tlo_parsing::type *type_of(const std::unique_ptr<vk::tlo_parsing::type_expr_base> &type_expr, const vk::tlo_parsing::tl_scheme *scheme) {
  if (auto *casted = type_expr->template as<vk::tlo_parsing::type_expr>()) {
    auto type_it = scheme->types.find(casted->type_id);
    if (type_it != scheme->types.end()) {
      return type_it->second.get();
    }
  }
  return nullptr;
}

vk::tlo_parsing::type *type_of(const vk::tlo_parsing::type_expr *type, const vk::tlo_parsing::tl_scheme *scheme) {
  auto type_it = scheme->types.find(type->type_id);
  if (type_it != scheme->types.end()) {
    return type_it->second.get();
  }
  return nullptr;
}

std::vector<std::string> get_optional_args_for_call(const std::unique_ptr<vk::tlo_parsing::combinator> &constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->is_optional()) {
      if (type_of(arg->type_expr)->is_integer_variable()) {
        res.emplace_back(arg->name);
      } else {
        kphp_assert(arg->is_type());
        res.emplace_back("std::move(" + arg->name + ")");
      }
    }
  }
  return res;
}

std::vector<std::string> get_template_params(const vk::tlo_parsing::combinator *constructor) {
  std::vector<std::string> typenames;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_TYPE) {
      kphp_assert(arg->is_optional());
      typenames.emplace_back(fmt_format("T{}", arg->var_num));
      typenames.emplace_back(fmt_format("inner_magic{}", arg->var_num));
    }
  }
  return typenames;
}

std::string get_template_declaration(const vk::tlo_parsing::combinator *constructor) {
  std::vector<std::string> typenames = get_template_params(constructor);
  if (typenames.empty()) {
    return "";
  }
  std::transform(typenames.begin(), typenames.end(), typenames.begin(), [](vk::string_view s) {
    if (s.starts_with("T")) {
      return "typename " + s;
    } else {
      return "unsigned int " + s;
    }
  });
  return "template<" + vk::join(typenames, ", ") + ">";
}

std::string get_template_definition(const vk::tlo_parsing::combinator *constructor) {
  std::vector<std::string> typenames = get_template_params(constructor);
  if (typenames.empty()) {
    return "";
  }
  return "<" + vk::join(typenames, ", ") + ">";
}

std::string get_php_runtime_type(const vk::tlo_parsing::combinator *c, bool wrap_to_class_instance, const std::string &type_name) {
  auto *c_from_renamed = get_this_from_renamed_tl_scheme(c);
  std::string res;
  std::string name = type_name.empty() ? c_from_renamed->name : type_name;
  if (c_from_renamed->is_constructor() && is_type_dependent(c_from_renamed, tl)) {
    std::vector<std::string> template_params;
    for (const auto &arg : c_from_renamed->args) {
      if (arg->is_type()) {
        kphp_assert(arg->is_optional());
        template_params.emplace_back(fmt_format("typename T{}::PhpType", arg->var_num));
      }
    }
    std::string template_suf = template_params.empty() ? "" : "<" + vk::join(template_params, ", ") + ">";
    std::replace(name.begin(), name.end(), '.', '_');
    res = "typename " + name + "__" + template_suf + "::type";
  } else {
    std::string php_namespace = get_php_namespace(name);
    std::replace(name.begin(), name.end(), '.', '_');
    res = G->settings().tl_classname_prefix.get() + php_namespace + "$" + (c_from_renamed->is_constructor() ? "Types$" : "Functions$") + name;
  }
  if (wrap_to_class_instance) {
    return fmt_format("class_instance<{}>", res);
  }
  return res;
}

std::string get_php_runtime_type(const vk::tlo_parsing::type *t) {
  auto *t_from_renamed = get_this_from_renamed_tl_scheme(t);
  if (t_from_renamed->is_polymorphic()) { // then we'll use a type name instead of the constructor name
    return get_php_runtime_type(t_from_renamed->constructors[0].get(), true, t_from_renamed->name);
  }
  return get_php_runtime_type(t_from_renamed->constructors[0].get(), true);
}

std::string get_tl_object_field_access(const std::unique_ptr<vk::tlo_parsing::arg> &arg, field_rw_type rw_type) {
  kphp_assert(!arg->name.empty());
  if (arg->is_fields_mask_optional()) {
    if (auto *type_var = arg->type_expr->as<vk::tlo_parsing::type_var>()) {
      std::string serializer_type_name = "T" + std::to_string(type_var->var_num);
      std::string field_access_type = rw_type == field_rw_type::READ ? "read" : "write";
      return fmt_format("get_serialization_target_from_optional_field<{}, FieldAccessType::{}>(tl_object->${})", serializer_type_name, field_access_type,
                        arg->name);
    } else {
      vk::tlo_parsing::type *type = type_of(arg->type_expr);
      kphp_assert(type);
      if (is_tl_type_wrapped_to_Optional(type)) {
        return fmt_format("tl_object->${}.{}", arg->name, rw_type == field_rw_type::READ ? "val()" : "ref()");
      }
    }
  }
  return fmt_format("tl_object->${}", arg->name);
}

bool is_type_dependent(const vk::tlo_parsing::combinator *constructor, const vk::tlo_parsing::tl_scheme *scheme) {
  kphp_assert(constructor->is_constructor());
  for (const auto &arg : constructor->args) {
    auto *arg_type = type_of(arg->type_expr, scheme);
    if (arg_type && arg_type->name == "Type") {
      return true;
    }
  }
  return false;
}

bool is_type_dependent(const vk::tlo_parsing::type *type, const vk::tlo_parsing::tl_scheme *scheme) {
  return is_type_dependent(type->constructors[0].get(), scheme);
}
} // namespace tl2cpp

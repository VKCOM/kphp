#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

#include "common-php-functions.h"

namespace tl2cpp {
const std::string T_TYPE = "Type";
const std::unordered_set<std::string> CUSTOM_IMPL_TYPES
  {"#", T_TYPE, "Int", "Long", "Double", "Float", "String", "Bool", "True", "False", "Vector", "Maybe", "Tuple",
   "Dictionary", "DictionaryField", "IntKeyDictionary", "IntKeyDictionaryField", "LongKeyDictionary", "LongKeyDictionaryField"};

static const std::string VK_name_prefix = "VK\\";
vk::tl::tl_scheme *tl;
const vk::tl::combinator *cur_combinator;

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

std::string get_magic_storing(const vk::tl::type_expr_base *arg_type_expr) {
  if (auto arg_as_type_expr = arg_type_expr->template as<vk::tl::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      return fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(type_of(arg_as_type_expr)->id));
    }
  } else if (auto arg_as_type_var = arg_type_expr->template as<vk::tl::type_var>()) {
    return fmt_format("store_magic_if_not_bare(inner_magic{});", arg_as_type_var->var_num);
  }
  return "";
}

std::string get_magic_fetching(const vk::tl::type_expr_base *arg_type_expr, const std::string &error_msg) {
  if (auto arg_as_type_expr = arg_type_expr->template as<vk::tl::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      return fmt_format(R"(fetch_magic_if_not_bare({:#010x}, "{}");)", static_cast<unsigned int>(type_of(arg_as_type_expr)->id), error_msg);
    }
  } else if (auto arg_as_type_var = arg_type_expr->template as<vk::tl::type_var>()) {
    for (const auto &arg : cur_combinator->args) {
      if (auto casted = arg->type_expr->as<vk::tl::type_var>()) {
        if (arg->is_forwarded_function() && casted->var_num == arg_as_type_var->var_num) {
          return "";
        }
      }
    }
    return fmt_format( R"(fetch_magic_if_not_bare(inner_magic{}, "{}");)", arg_as_type_var->var_num, error_msg);
  }
  return "";
}

// Переименованная тл схема нужна во всех функциях, которые по какому-то tl объекту достают php объект или наоборот. Примерный список таких:
// 1) TlTemplateTypeHelpers
// 2) get_php_runtime_type(...)
// 3) get_php_class_of_tl_*(...)
vk::tl::type *get_this_from_renamed_tl_scheme(const vk::tl::type *t) {
  return G->get_tl_classes().get_scheme()->get_type_by_magic(t->id);
}

vk::tl::combinator *get_this_from_renamed_tl_scheme(const vk::tl::combinator *c) {
  return c->is_constructor() ?
         G->get_tl_classes().get_scheme()->get_constructor_by_magic(c->id) : G->get_tl_classes().get_scheme()->get_function_by_magic(c->id);
}

std::string get_php_namespace(const std::string &tl_name) {
  size_t dot_pos = tl_name.find('.');
  return dot_pos == std::string::npos ? vk::tl::PhpClasses::common_engine_namespace() : tl_name.substr(0, dot_pos);
}

// найти по php-классу соответствующий конструктор в tl-схеме
// например, class VK\TL\messages\Types\messages_chatInfo => вернёт messages.chatInfo из схемы
// это может быть и специализация конструктора зависимого типа: Types\vectorTotal__int вернёт tl-ный vectorTotal
vk::tl::combinator *get_tl_constructor_of_php_class(ClassPtr klass) {
  const auto &tl_scheme = G->get_tl_classes().get_scheme();

  const auto &php_representations = G->get_tl_classes().get_php_classes().all_classes;
  auto constructor_php_repr_iter = php_representations.find(klass->name.substr(VK_name_prefix.length()));
  kphp_assert(constructor_php_repr_iter != php_representations.end());

  const auto &constructor_magic_iter = tl_scheme->magics.find(constructor_php_repr_iter->second.get().tl_name);
  kphp_assert(constructor_magic_iter != tl_scheme->magics.end());

  vk::tl::combinator *res = tl_scheme->get_constructor_by_magic(constructor_magic_iter->second);
  kphp_assert(res);
  return res;
}

// найти по php-интерфейсу соответствующий tl-ный ПОЛИМОРФНЫЙ тип
// поскольку если тип не полиморфный, то вместо типов конструкторы (например, class messages_chatInfo)
// то эта функция только для полиморфных типов (например, interface memcache_Value)
vk::tl::type *get_tl_type_of_php_class(ClassPtr interface) {
  kphp_assert(is_php_class_a_tl_polymorphic_type(interface));
  const auto &tl_scheme = G->get_tl_classes().get_scheme();

  const auto &tl_type_php_representations = G->get_tl_classes().get_php_classes().types;
  auto php_repr_iter = tl_type_php_representations.find(interface->name.substr(VK_name_prefix.length()));
  kphp_assert(php_repr_iter != tl_type_php_representations.end());

  auto tl_magic_iter = tl_scheme->magics.find(php_repr_iter->second.type_representation->tl_name);
  kphp_assert(tl_magic_iter != tl_scheme->magics.end());

  auto res_tl_type = tl_scheme->get_type_by_magic(tl_magic_iter->second);
  kphp_assert(res_tl_type);
  return res_tl_type;
}

// если конструктор от зависимого типа, например vectorTotal {t:Type} ..., то одному tl-конструктору соответствуют
// много php-специализаций: vectorTotal__int и др.
// данная функция получает конкретную специализацию такого конструктора с raw-суффиксом имени php-класса (e.g. "__int")
ClassPtr get_php_class_of_tl_constructor_specialization(const vk::tl::combinator *c, const std::string &specialization_suffix) {
  auto c_from_renamed = get_this_from_renamed_tl_scheme(c);
  std::string php_namespace = get_php_namespace(c_from_renamed->name);
  std::string php_class_name = G->env().get_tl_namespace_prefix() + php_namespace + "\\Types\\" + replace_characters(c_from_renamed->name, '.', '_') + specialization_suffix;
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

// одному tl-конструктор соответствует либо один php-класс (в случае простых конструкторов, например likes.item)
// либо несколько, если это конструктор с зависимыми типами, и появляются vectorTotal__int и другие специализации
std::vector<ClassPtr> get_all_php_classes_of_tl_constructor(const vk::tl::combinator *c) {
  auto c_from_renamed = get_this_from_renamed_tl_scheme(c); // Для консистентности
  return _get_all_php_classes_by_tl_magic(c_from_renamed->id, false);
}

// одному tl-типу соответствует либо один класс/интерфейс (в случае простых типов, например likes.Item)
// либо несколько, если это тип с зависимыми типами, и появлятся Either_string_Vertex и другие специализации
std::vector<ClassPtr> get_all_php_classes_of_tl_type(const vk::tl::type *t) {
  auto t_from_renamed = get_this_from_renamed_tl_scheme(t); // Для консистентности
  return _get_all_php_classes_by_tl_magic(t_from_renamed->id, t_from_renamed->is_polymorphic());
}

// если тип с зависимыми типами, например VectorTotal t, то одному tl-типу соответствуют
// много php-специализаций vectorTotal__int и т.п. (неполиморфные типы сращиваются с конструкторами в один класс как обычно)
ClassPtr get_php_class_of_tl_type_specialization(const vk::tl::type *t, const std::string &specialization_suffix) {
  auto t_from_renamed = get_this_from_renamed_tl_scheme(t);
  kphp_assert(is_type_dependent(t_from_renamed, G->get_tl_classes().get_scheme().get()));
  std::string php_namespace = get_php_namespace(t_from_renamed->name);
  std::string lookup_name = t_from_renamed->is_polymorphic() ? t_from_renamed->name : t_from_renamed->constructors[0]->name;
  std::string php_class_name = G->env().get_tl_namespace_prefix() + php_namespace + "\\Types\\" + replace_characters(lookup_name, '.', '_') + specialization_suffix;
  return G->get_class(php_class_name);
}

// tl-функции 'messages.getChatInfo' соответствует php class VK\TL\messages\Functions\messages_getChatInfo
// если этот класс достижим компилятором — типизированный вариант для messages.getChatInfo нужен
// (а если нет, то эта tl-функция типизированно гарантированно не вызывается)
ClassPtr get_php_class_of_tl_function(const vk::tl::combinator *f) {
  auto f_from_renamed = get_this_from_renamed_tl_scheme(f);
  std::string php_namespace = get_php_namespace(f_from_renamed->name);
  std::string php_class_name = G->env().get_tl_namespace_prefix() + php_namespace + "\\Functions\\" + replace_characters(f_from_renamed->name, '.', '_');
  return G->get_class(php_class_name);
}

// и наоборот: классу VK\TL\messages\Functions\messages_getChatInfo соответствует tl-функция messages.getChatInfo
std::string get_tl_function_name_of_php_class(ClassPtr klass) {
  kphp_assert(is_php_class_a_tl_function(klass));
  size_t functions_ns_pos = vk::string_view{klass->name}.find("Functions\\");
  std::string after_ns_functions = static_cast<std::string>(vk::string_view{klass->name}.substr(functions_ns_pos + 10));
  size_t underscore_pos = klass->name.find('_');
  if (underscore_pos == std::string::npos) {
    return after_ns_functions;
  }
  // Если есть '_', то надо понять какой из них заменить на '.', тк есть имена и с '_' до '.', и после (expr.earth_distance, smart_alerts.sendMessage)
  std::string tmp = klass->name.substr(G->env().get_tl_namespace_prefix().length());
  std::string php_namespace = tmp.substr(0, tmp.find('\\'));
  if (php_namespace == vk::tl::PhpClasses::common_engine_namespace()) {
    return after_ns_functions;
  }
  after_ns_functions.at(php_namespace.length()) = '.';
  return after_ns_functions;
}

// у tl-функции 'messages.getChatInfo' есть типизированный результат php class VK\TL\messages\Functions\messages_getChatInfo_result
// по идее, достижима функция <=> достижим результат
ClassPtr get_php_class_of_tl_function_result(const vk::tl::combinator *f) {
  auto f_from_renamed = get_this_from_renamed_tl_scheme(f);
  std::string php_namespace = get_php_namespace(f_from_renamed->name);
  std::string php_class_name = G->env().get_tl_namespace_prefix() + php_namespace + "\\Functions\\" + replace_characters(f_from_renamed->name, '.', '_') + "_result";
  return G->get_class(php_class_name);
}

// классы VK\TL\*\Functions\* implements RpcFunction — это те, что соответствуют tl-функциям
bool is_php_class_a_tl_function(ClassPtr klass) {
  return klass->is_tl_class && klass->implements.size() == 1 && vk::string_view{klass->implements.front()->name}.ends_with("RpcFunction");
}

// классы VK\TL\*\Types\* — не интерфейсные — соответствуют конструкторам
// (или неполиморфмным типам, единственный конструктор которых заинлайнен)
bool is_php_class_a_tl_constructor(ClassPtr klass) {
  return klass->is_tl_class && klass->is_class() && klass->name.find("Types\\") != std::string::npos;
}

// если в tl-схеме объявлены сложные [массивы], то для типизации для каждого массива есть отдельный php-класс
// пример: isearch.typeInfo n:# data:n*[type:int probability:double] = isearch.TypeInfo
// из него выделился class isearch\isearch_typeInfo_arg2_item { int type, double probability }
bool is_php_class_a_tl_array_item(ClassPtr klass) {
  return klass->is_tl_class && klass->is_class() && klass->name.find("Types\\") != std::string::npos &&
         klass->name.find("_arg") != std::string::npos && klass->name.find("_item") != std::string::npos;
}

bool is_tl_type_a_php_array(const vk::tl::type *t) {
  return t->id == TL_VECTOR || t->id == TL_TUPLE || t->id == TL_DICTIONARY ||
         t->id == TL_INT_KEY_DICTIONARY || t->id == TL_LONG_KEY_DICTIONARY;
}

bool is_tl_type_wrapped_to_Optional(const vk::tl::type *type) {
  // [fields_mask.n? | Maybe] [int|string|array|double|bool] -- с Optional
  // [fields_mask.n? | Maybe] [class_instance<T>|Optional<T>|var] -- без Optional
  return is_tl_type_a_php_array(type) || vk::any_of_equal(type->id, TL_INT, TL_DOUBLE, TL_FLOAT, TL_STRING) || type->name == "Bool" || type->is_integer_variable();
}

// классы VK\TL\Types\* — интерфейсы — это полиморфные типы, конструкторы которых классы implements его
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass) {
  return klass->is_tl_class && klass->is_interface() && klass->name.find("Types\\") != std::string::npos;
}

bool is_magic_processing_needed(const vk::tl::type_expr *type_expr) {
  const auto &type = tl->types[type_expr->type_id];
  // полиморфные типы процессят magic внутри себя, а не снаружи, а для "#" вообще magic'а нет никогда
  bool handles_magic_inside = type->is_integer_variable() ? true : type->is_polymorphic();
  bool is_used_as_bare = type_expr->is_bare();
  // означает: тип полиморфный или с % — не нужно читать снаружи его magic
  return !(handles_magic_inside || is_used_as_bare);
}

vk::tl::type *type_of(const std::unique_ptr<vk::tl::type_expr_base> &type_expr, const vk::tl::tl_scheme *scheme) {
  if (auto casted = type_expr->template as<vk::tl::type_expr>()) {
    auto type_it = scheme->types.find(casted->type_id);
    if (type_it != scheme->types.end()) {
      return type_it->second.get();
    }
  }
  return nullptr;
}

vk::tl::type *type_of(const vk::tl::type_expr *type, const vk::tl::tl_scheme *scheme) {
  auto type_it = scheme->types.find(type->type_id);
  if (type_it != scheme->types.end()) {
    return type_it->second.get();
  }
  return nullptr;
}

std::vector<std::string> get_optional_args_for_call(const std::unique_ptr<vk::tl::combinator> &constructor) {
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

std::vector<std::string> get_template_params(const vk::tl::combinator *constructor) {
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

std::string get_template_declaration(const vk::tl::combinator *constructor) {
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

std::string get_template_definition(const vk::tl::combinator *constructor) {
  std::vector<std::string> typenames = get_template_params(constructor);
  if (typenames.empty()) {
    return "";
  }
  return "<" + vk::join(typenames, ", ") + ">";
}


std::string get_php_runtime_type(const vk::tl::combinator *c, bool wrap_to_class_instance, const std::string &type_name) {
  auto c_from_renamed = get_this_from_renamed_tl_scheme(c);
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
    res = G->env().get_tl_classname_prefix() + php_namespace + "$" + (c_from_renamed->is_constructor() ? "Types$" : "Functions$") + name;
  }
  if (wrap_to_class_instance) {
    return fmt_format("class_instance<{}>", res);
  }
  return res;
}

std::string get_php_runtime_type(const vk::tl::type *t) {
  auto t_from_renamed = get_this_from_renamed_tl_scheme(t);
  if (t_from_renamed->is_polymorphic()) {    // тогда пользуемся именем типа, а не конструктора
    return get_php_runtime_type(t_from_renamed->constructors[0].get(), true, t_from_renamed->name);
  }
  return get_php_runtime_type(t_from_renamed->constructors[0].get(), true);
}

std::string get_tl_object_field_access(const std::unique_ptr<vk::tl::arg> &arg, field_rw_type rw_type) {
  kphp_assert(!arg->name.empty());
  std::string optional_inner_access;
  if (arg->is_fields_mask_optional() && is_tl_type_wrapped_to_Optional(type_of(arg->type_expr))) {
    optional_inner_access = rw_type == field_rw_type::READ ? ".val()" : ".ref()";
  }
  return fmt_format("tl_object->${}", arg->name) + optional_inner_access;
}

bool is_type_dependent(const vk::tl::combinator *constructor, const vk::tl::tl_scheme *scheme) {
  kphp_assert(constructor->is_constructor());
  for (const auto &arg : constructor->args) {
    auto arg_type = type_of(arg->type_expr, scheme);
    if (arg_type && arg_type->name == "Type") {
      return true;
    }
  }
  return false;
}

bool is_type_dependent(const vk::tl::type *type, const vk::tl::tl_scheme *scheme) {
  return is_type_dependent(type->constructors[0].get(), scheme);
}
}

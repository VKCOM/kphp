#include "compiler/code-gen/files/tl2cpp.h"

#include <string>

#include "auto/TL/constants.h"
#include "common/algorithms/string-algorithms.h"
#include "common/tlo-parsing/flat-optimization.h"
#include "common/tlo-parsing/replace-anonymous-args.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"

/* При генерации выделены 3 основные сущности, у каждой из которых есть методы store и fetch:
 * 1) Функция     - entry point любого tl запроса. Из него начинается де\сериализация любого запроса.
 *                  Имеет поля/аргументы (имена таких сгенеренных структур начинаются с "f_").
 * 2) Тип         - свойтсво поля любого комбинатора (Функции или Конструктора).
 *                  де\сериализация любого поля начинается с вызова store/fetch у соответствующего Типа (struct t_...).
 *                  Реализация представляет собой switch по magic'у и перевызов соответствующего метода Конструктора.
 * 3) Конструктор - один из вариантов наборов данных для конкретного типа ("слагаемое" из суммы типов).
 *                  store/fetch у Конструктора вызывается ТОЛЬКО из store/fetch соответствующего типа.
 *                  Имена генеренных структур начинаются с "c_".
 *
 * @@@ Структуры для генерации @@@
 * Наименования начинаются с названия сущности, для которой генерится код (Function, Type, Constructor, Combinator, TypeExpr)
 * Структуры, с вызова compile() которых, всегда начинается генерация:
 * 1) <*>Decl - генерация объявления структуры.
 * 2) <*>Def  - генерация определения структуры.
 * где <*> - Function, Type или Constructor
 *
 * После всегда вызывается compile() следующих структур:
 * 1) <*>Store - генерация функции store()
 * 2) <*>Fetch - генерация функции fetch()
 * где <*> - Combinator или Type
 * Combinator - это функция или конструктор.
 * Они имеют одинаковую семантику и поэтому store/fetch для них генерится общим кодом.
 *
 * Дальше всегда вызывается compile() у:
 * 1) TypeExprStore
 * 2) TypeExprFetch
 * Генерации фетчинга или сторинга конкретного типового выражения (например Maybe (%tree_stats.PeriodsResult fields_mask))
 */
namespace tl_gen {
static const std::string T_TYPE = "Type";

using vk::tl::FLAG_DEFAULT_CONSTRUCTOR;
using vk::tl::FLAG_NOCONS;

static vk::tl::tl_scheme *tl;
static const vk::tl::combinator *cur_combinator;
static const std::string VK_name_prefix = "VK\\";

const std::unordered_set<std::string> CUSTOM_IMPL_TYPES   // из tl_builtins.h
  {"#", T_TYPE, "Int", "Long", "Double", "String", "Bool", "True", "False", "Vector", "Maybe", "Tuple",
   "Dictionary", "DictionaryField", "IntKeyDictionary", "IntKeyDictionaryField", "LongKeyDictionary", "LongKeyDictionaryField"};

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

bool is_tl_type_wrapped_to_OrFalse(const vk::tl::type *type) {
  // Maybe<int|string|array|double> -- с OrFalse
  // Maybe<class_instance|bool|OrFalse|var> -- без OrFalse
  return is_tl_type_a_php_array(type) || vk::any_of_equal(type->id, TL_INT, TL_DOUBLE, TL_STRING) || type->is_integer_variable();
}

// классы VK\TL\Types\* — интерфейсы — это полиморфные типы, конструкторы которых классы implements его
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass) {
  return klass->is_tl_class && klass->is_interface() && klass->name.find("Types\\") != std::string::npos;
}
// Вспомогательные функции для генерации
namespace {
vk::tl::type *type_of(const std::unique_ptr<vk::tl::type_expr_base> &type_expr, const vk::tl::tl_scheme *scheme = tl) {
  if (auto casted = type_expr->template as<vk::tl::type_expr>()) {
    auto type_it = scheme->types.find(casted->type_id);
    if (type_it != scheme->types.end()) {
      return type_it->second.get();
    }
  }
  return nullptr;
}

vk::tl::type *type_of(const vk::tl::type_expr *type, const vk::tl::tl_scheme *scheme = tl) {
  auto type_it = scheme->types.find(type->type_id);
  if (type_it != scheme->types.end()) {
    return type_it->second.get();
  }
  return nullptr;
}

bool is_magic_processing_needed(const vk::tl::type_expr *type_expr) {
  const auto &type = tl->types[type_expr->type_id];
  // полиморфные типы процессят magic внутри себя, а не снаружи, а для "#" вообще magic'а нет никогда
  bool handles_magic_inside = type->is_integer_variable() ? true : type->is_polymorphic();
  bool is_used_as_bare = type_expr->is_bare();
  // означает: тип полиморфный или с % — не нужно читать снаружи его magic
  return !(handles_magic_inside || is_used_as_bare);
}

std::vector<std::string> get_not_optional_fields_masks(const vk::tl::combinator *constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1 && type_of(arg->type_expr)->is_integer_variable() && !arg->is_optional()) {
      res.emplace_back(arg->name);
    }
  }
  return res;
}

std::vector<std::string> get_optional_args_for_call(const std::unique_ptr<vk::tl::combinator> &constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->is_optional()) {
      if (type_of(arg->type_expr)->is_integer_variable()) {
        res.emplace_back(arg->name);
      } else {
        kphp_assert(arg->is_type(tl));
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


std::string get_php_runtime_type(const vk::tl::combinator *c, bool wrap_to_class_instance = false, const std::string &type_name = "") {
  auto c_from_renamed = get_this_from_renamed_tl_scheme(c);
  std::string res;
  std::string name = type_name.empty() ? c_from_renamed->name : type_name;
  if (c_from_renamed->is_constructor() && is_type_dependent(c_from_renamed, tl)) {
    std::vector<std::string> template_params;
    for (const auto &arg : c_from_renamed->args) {
      if (arg->is_type(tl)) {
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

enum class field_rw_type {
  READ,
  WRITE
};

std::string get_tl_object_field_access(const std::unique_ptr<vk::tl::arg> &arg, field_rw_type rw_type) {
  kphp_assert(!arg->name.empty());
  std::string or_false_inner_access;
  if (arg->is_fields_mask_optional() && is_tl_type_wrapped_to_OrFalse(type_of(arg->type_expr))) {
    or_false_inner_access = rw_type == field_rw_type::READ ? ".val()" : ".ref()";
  }
  return fmt_format("tl_object->${}", arg->name) + or_false_inner_access;
}
} // namespace

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

static std::set<std::string> tl_const_vars;

static std::string cpp_tl_const_str(std::string tl_name) {
  std::replace(tl_name.begin(), tl_name.end(), '.', '$');
  return "tl_str$" + tl_name;
}

static std::string register_tl_const_str(const std::string &tl_name) {
  tl_const_vars.insert(tl_name);
  return cpp_tl_const_str(tl_name);
}

static int hash_tl_const_str(const std::string &tl_name) {
  return string_hash(tl_name.c_str(), (int)tl_name.size());
}

std::string cpp_tl_struct_name(const char *prefix, std::string tl_name, const std::string &template_args_postfix) {
  std::replace(tl_name.begin(), tl_name.end(), '.', '_');
  return std::string(prefix) + tl_name + template_args_postfix;
}

// Пример сгенеренного кода:
/*
 * Нетипизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::store(const var &tl_object) {
  const string &c_name = tl_arr_get(tl_object, tl_str$_, 0, -2147483553).to_string();
  if (c_name == tl_str$left) {
    f$store_int(0x0a29cd5d);
    c_left<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else if (c_name == tl_str$right) {
    f$store_int(0xdf3ecb3b);
    c_right<T0, inner_magic0, T1, inner_magic1>::store(tl_object, std::move(X), std::move(Y));
  } else {
    CurrentProcessingQuery::get().raise_storing_error("Invalid constructor %s of type %s", c_name.c_str(), "Either");
  }
}
 * Типизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::typed_store(const PhpType &tl_object) {
  if (f$is_a<typename left__<typename T0::PhpType, typename T1::PhpType>::type>(tl_object)) {
    f$store_int(0x0a29cd5d);
    const typename left__<typename T0::PhpType, typename T1::PhpType>::type *conv_obj = tl_object.template cast_to<typename left__<typename T0::PhpType, typename T1::PhpType>::type>().get();
    c_left<T0, inner_magic0, T1, inner_magic1>::typed_store(conv_obj, std::move(X), std::move(Y));
  } else if (f$is_a<typename right__<typename T0::PhpType, typename T1::PhpType>::type>(tl_object)) {
    f$store_int(0xdf3ecb3b);
    const typename right__<typename T0::PhpType, typename T1::PhpType>::type *conv_obj = tl_object.template cast_to<typename right__<typename T0::PhpType, typename T1::PhpType>::type>().get();
    c_right<T0, inner_magic0, T1, inner_magic1>::typed_store(conv_obj, std::move(X), std::move(Y));
  } else {
    CurrentProcessingQuery::get().raise_storing_error("Invalid constructor %s of type %s", tl_object.get_class(), "Either");
  }
}
*/
struct TypeStore {
  const vk::tl::type *type;
  std::string template_str;
  bool typed_mode;

  TypeStore(const vk::tl::type *type, string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const {
    auto store_params = get_optional_args_for_call(type->constructors[0]);
    store_params.insert(store_params.begin(),
                        typed_mode ? (type->is_polymorphic() ? "conv_obj" : "tl_object.get()") : "tl_object");
    std::string store_call = (typed_mode ? "typed_store(" : "store(") + vk::join(store_params, ", ") + ");";

    if (typed_mode) {
      W << "if (tl_object.is_null()) " << BEGIN
        << "CurrentProcessingQuery::get().raise_storing_error(\"Instance expected, but false given while storing tl type\");" << NL
        << "return;" << NL
        << END << NL;
    }
    // неполиморфные типы — 1 конструктор — просто форвардят ::store() в конструктор, без всяких magic
    if (!type->is_polymorphic()) {
      W << cpp_tl_struct_name("c_", type->constructors[0]->name, template_str) << "::" << store_call << NL;
      return;
    }

    auto default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
    // для полиморфных типов:
    // типизированно: if'ы с dynamic_cast, примерно как интерфейсные методы
    if (typed_mode) {
      bool first = true;
      for (const auto &c : type->constructors) {
        W << (first ? "if " : " else if ");
        first = false;

        std::string cpp_class = get_php_runtime_type(c.get());
        W << "(f$is_a<" << cpp_class << ">(tl_object)) " << BEGIN;
        if (c.get() != default_constructor) {
          W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(c->id)) << NL;
        }
        W << "const " << cpp_class << " *conv_obj = tl_object.template cast_to<" << cpp_class << ">().get();" << NL;
        W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << store_call << NL << END;
      }
      W << (first ? "" : " else ") << BEGIN
        << "CurrentProcessingQuery::get().raise_storing_error(\"Invalid constructor %s of type %s\", "
        << "tl_object.get_class(), \"" << type->name << "\");" << NL
        << END << NL;

      // нетипизированно: if'ы с проверкой constructor_name
    } else {
      bool first = true;
      W << "const string &c_name = tl_arr_get(tl_object, "
        << register_tl_const_str("_") << ", 0, " << hash_tl_const_str("_") << ").to_string();" << NL;
      for (const auto &c : type->constructors) {
        W << (first ? "if " : " else if ");
        first = false;

        W << "(c_name == " << register_tl_const_str(c->name) << ") " << BEGIN;
        if (c.get() != default_constructor) {
          W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(c->id)) << NL;
        }
        W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << store_call << NL << END;
      }
      W << (first ? "" : " else ") << BEGIN
        << "CurrentProcessingQuery::get().raise_storing_error(\"Invalid constructor %s of type %s\", "
        << "c_name.c_str(), \"" << type->name << "\");" << NL
        << END << NL;
    }
  }
};


// Пример сгенеренного кода:
/*
 * Нетипизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
array<var> t_Either<T0, inner_magic0, T1, inner_magic1>::fetch() {
  array<var> result;
  CHECK_EXCEPTION(return result);
  auto magic = static_cast<unsigned int>(f$fetch_int());
  switch(magic) {
    case 0x0a29cd5d: {
      result = c_left<T0, inner_magic0, T1, inner_magic1>::fetch(std::move(X), std::move(Y));
      result.set_value(tl_str$_, tl_str$left, -2147483553);
      break;
    }
    case 0xdf3ecb3b: {
      result = c_right<T0, inner_magic0, T1, inner_magic1>::fetch(std::move(X), std::move(Y));
      result.set_value(tl_str$_, tl_str$right, -2147483553);
      break;
    }
    default: {
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
  return result;
}
 * Типизированно:
template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
void t_Either<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(PhpType &tl_object) {
  CHECK_EXCEPTION(return);
  auto magic = static_cast<unsigned int>(f$fetch_int());
  switch(magic) {
    case 0x0a29cd5d: {
      class_instance<typename left__<typename T0::PhpType, typename T1::PhpType>::type> result;
      result.alloc();
      c_left<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(result.get(), std::move(X), std::move(Y));
      tl_object = result;
      break;
    }
    case 0xdf3ecb3b: {
      class_instance<typename right__<typename T0::PhpType, typename T1::PhpType>::type> result;
      result.alloc();
      c_right<T0, inner_magic0, T1, inner_magic1>::typed_fetch_to(result.get(), std::move(X), std::move(Y));
      tl_object = result;
      break;
    }
    default: {
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type Either: 0x%08x", magic);
    }
  }
}
*/
struct TypeFetch {
  const vk::tl::type *type;
  std::string template_str;
  bool typed_mode;

  inline TypeFetch(const vk::tl::type *type, string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  inline void compile(CodeGenerator &W) const {
    auto fetch_params = get_optional_args_for_call(type->constructors[0]);
    std::string fetch_call;
    if (!typed_mode) {
      fetch_call = "fetch(" + vk::join(fetch_params, ", ") + ");";
    } else {
      fetch_params.insert(fetch_params.begin(), "result.get()");
      fetch_call = "typed_fetch_to(" + vk::join(fetch_params, ", ") + ");";
    }

    // типы с однозначным конструктором фетчатся просто — делегируют fetch в конструктор
    if (!type->is_polymorphic()) {
      auto &constructor = type->constructors.front();
      if (typed_mode) {
        W << "CHECK_EXCEPTION(return);" << NL;
        W << get_php_runtime_type(constructor.get(), true) << " result;" << NL;
        W << "result.alloc();" << NL;
        // во все c_*.typed_fetch_to() приходит уже аллоцированный объект.
        // Он всегда аллоцируется в t_*.typed_fetch_to().
        W << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
        W << "tl_object = result;" << NL;
      } else {
        W << "CHECK_EXCEPTION(return array<var>());" << NL;
        W << "return " << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
      }
      return;
    }

    // полиформные типы фетчатся сложнее: сначала magic, потом switch(magic) на каждый конструктор
    if (!typed_mode) {
      W << "array<var> result;" << NL;
    }
    W << "CHECK_EXCEPTION(return" << (typed_mode ? "" : " result") << ");" << NL;
    auto default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
    bool has_name = type->constructors_num > 1 && !(type->flags & FLAG_NOCONS);
    if (default_constructor != nullptr) {
      W << "int pos = tl_parse_save_pos();" << NL;
    }
    W << "auto magic = static_cast<unsigned int>(f$fetch_int());" << NL;
    W << "switch(magic) " << BEGIN;
    for (const auto &c : type->constructors) {
      if (c.get() == default_constructor) {
        continue;
      }
      W << fmt_format("case {:#010x}: ", static_cast<unsigned int>(c->id)) << BEGIN;
      if (!typed_mode) {
        W << "result = " << cpp_tl_struct_name("c_", c->name, template_str) << "::" << fetch_call << NL;
        if (has_name) {
          W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(c->name) << ", " << hash_tl_const_str("_") << ");"
            << NL;
        }
      } else {
        W << get_php_runtime_type(c.get(), true) << " result;" << NL;
        W << "result.alloc();" << NL;
        W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << fetch_call << NL;
        W << "tl_object = result;" << NL;
      }
      W << "break;" << NL;
      W << END << NL;
    }
    W << "default: " << BEGIN;
    if (default_constructor != nullptr) {
      W << "tl_parse_restore_pos(pos);" << NL;
      if (!typed_mode) {
        W << "result = " << cpp_tl_struct_name("c_", default_constructor->name, template_str) << "::" << fetch_call << NL;
        if (has_name) {
          W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(default_constructor->name) << ", "
            << hash_tl_const_str("_") << ");" << NL;
        }
      } else {
        W << get_php_runtime_type(default_constructor, true) << " result;" << NL;
        W << "result.alloc();" << NL;
        W << cpp_tl_struct_name("c_", default_constructor->name, template_str) << "::" << fetch_call << NL;
        W << "tl_object = result;" << NL;
      }
    } else {
      W << "CurrentProcessingQuery::get().raise_fetching_error(\"Incorrect magic of type " << type->name << ": 0x%08x\", magic);" << NL;
    }
    W << END << NL;
    W << END << NL;
    if (!typed_mode) {
      W << "return result;" << NL;
    }
  }
};

std::pair<std::string, std::string> get_full_type_expr_str(vk::tl::expr_base *type_expr, const std::string &var_num_access);

std::string get_full_value(vk::tl::expr_base *type_expr, const std::string &var_num_access) {
  auto ans = get_full_type_expr_str(type_expr, var_num_access);
  return ans.second;
}

std::string get_full_type(vk::tl::expr_base *type_expr, const std::string &var_num_access) {
  auto ans = get_full_type_expr_str(type_expr, var_num_access);
  return ans.first;
}

std::string get_storer_call(const std::unique_ptr<vk::tl::arg> &arg, const std::string &var_num_access, bool typed_mode) {
  std::string target_expr = get_full_value(arg->type_expr.get(), var_num_access);
  if (!typed_mode) {
    return target_expr + fmt_format(".store(tl_arr_get(tl_object, {}, {}, {}))", register_tl_const_str(arg->name), arg->idx, hash_tl_const_str(arg->name));
  } else {
    return target_expr + fmt_format(".typed_store({})", get_tl_object_field_access(arg, field_rw_type::READ));
  }
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

// Структура для генерации store любого type expression'а
struct TypeExprStore {
  const std::unique_ptr<vk::tl::arg> &arg;
  std::string var_num_access;
  bool typed_mode;

  TypeExprStore(const std::unique_ptr<vk::tl::arg> &arg, std::string var_num_access, bool typed_mode = false) :
    arg(arg),
    var_num_access(std::move(var_num_access)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const {
    std::string magic_storing = get_magic_storing(arg->type_expr.get());
    if (!magic_storing.empty()) {
      W << magic_storing << NL;
    }
    W << get_storer_call(arg, var_num_access, typed_mode) << ";" << NL;
  }
};

/* Общий код для генерации store(...) для комбинаторов (то есть функций или контсрукторов).
 * Содержит основную логику.
 * Есть несколько частей:
 * 1) Обработка филд масок
    void c_hints_objectExt::store(const var& tl_object, int fields_mask) {
      (void)tl_object;
      t_Int().store(tl_arr_get(tl_object, tl_str$type, 2, -445613708));
      t_Int().store(tl_arr_get(tl_object, tl_str$object_id, 3, 65801733));
      if (fields_mask & (1 << 0)) {
        t_Double().store(tl_arr_get(tl_object, tl_str$rating, 4, -1130913585));
      }
      if (fields_mask & (1 << 1)) {
        t_String().store(tl_arr_get(tl_object, tl_str$text, 5, -193436300));
      }
    }
 * 2) Обработка восклицательных знаков:
    std::unique_ptr<tl_func_base> f_rpcProxy_diagonalTargets::store(const var& tl_object) {
      auto result_fetcher = make_unique_on_script_memory<f_rpcProxy_diagonalTargets>();
      (void)tl_object;
      f$store_int(0xee090e42);
      t_Int().store(tl_arr_get(tl_object, tl_str$offset, 2, -1913876069));
      t_Int().store(tl_arr_get(tl_object, tl_str$limit, 3, 492966325));
      auto _cur_arg = tl_arr_get(tl_object, tl_str$query, 4, 1563700686);
      string target_f_name = tl_arr_get(_cur_arg, tl_str$_, 0, -2147483553).as_string();
      if (!tl_storers_ht.has_key(target_f_name)) {
        CurrentProcessingQuery::get().raise_storing_error("Function %s not found in tl-scheme", target_f_name.c_str());
        return {};
      }
      const auto &storer_kv = tl_storers_ht.get_value(target_f_name);
      result_fetcher->X.fetcher = storer_kv(_cur_arg);
      return std::move(result_fetcher);
    }
 * 3) Обработка основной части типового выражения в TypeExprStore / Fetch
*/
struct CombinatorStore {
  const vk::tl::combinator *combinator;
  bool typed_mode;

  inline explicit CombinatorStore(const vk::tl::combinator *combinator, bool typed_mode = false) :
    combinator(combinator),
    typed_mode(typed_mode) {}

  inline void compile(CodeGenerator &W) const {
    cur_combinator = combinator;
    auto var_num_access = (combinator->is_function() ? "result_fetcher->" : "");
    // Создаем локальные переменные для хранения филд масок, не являющихся параметрами типа
    if (combinator->is_constructor()) {
      auto fields_masks = get_not_optional_fields_masks(combinator);
      for (const auto &item : fields_masks) {
        W << "int " << item << "{0}; (void) " << item << ";" << NL;
      }
    }
    W << "(void)tl_object;" << NL;
    if (combinator->is_function()) {
      W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(combinator->id)) << NL;
    }
    for (const auto &arg : combinator->args) {
      // Если аргумент является параметром типа (обрамлен в {}), пропускаем его
      // Все такие аргументы хранятся в типе, как поля структуры
      if (arg->is_optional()) {
        continue;
      }
      // Если поле необязательное и зависит от филд маски
      if (arg->is_fields_mask_optional()) {
        // 2 случая:
        //      1) Зависимость от значения, которым параметризуется тип (имеет смысл только для конструкторов)
        //      2) Зависимость от значения, которое получаем из явных аргументов
        W << fmt_format("if ({}{} & (1 << {})) ",
                        var_num_access,
                        combinator->get_var_num_arg(arg->exist_var_num)->name,
                        arg->exist_var_bit) << BEGIN;
        // полиморфно обрабатываются, так как запоминаем их все либо в локальные переменны либо в поля структуры
        if (typed_mode) {
          // Проверяем, что не забыли проставить поле под филд маской
          std::string value_check = get_value_absence_check_for_optional_arg(arg.get());
          if (!value_check.empty()) {
            W << "if (" << value_check << ") " << BEGIN;
            W << fmt_format(R"(CurrentProcessingQuery::get().raise_storing_error("Optional field %s of %s is not set, but corresponding fields mask bit is set", "{}", "{}");)",
                            arg->name, combinator->name) << NL;
            W << "return" << (combinator->is_function() ? " {};" : ";") << NL;
            W << END << NL;
          }
        }
      }
      if (!arg->is_forwarded_function()) {
        W << TypeExprStore(arg, var_num_access, typed_mode);
      }
      //Обработка восклицательного знака
      if (arg->is_forwarded_function()) {
        kphp_assert(combinator->is_function());
        auto as_type_var = arg->type_expr->as<vk::tl::type_var>();
        kphp_assert(as_type_var);
        if (!typed_mode) {
          W << "auto _cur_arg = "
            << fmt_format("tl_arr_get(tl_object, {}, {}, {})", register_tl_const_str(arg->name), arg->idx, hash_tl_const_str(arg->name))
            << ";" << NL;
          W << "string target_f_name = " << fmt_format("tl_arr_get(_cur_arg, {}, 0, {}).as_string()", register_tl_const_str("_"), hash_tl_const_str("_"))
            << ";" << NL;
          W << "if (!tl_storers_ht.has_key(target_f_name)) " << BEGIN
            << "CurrentProcessingQuery::get().raise_storing_error(\"Function %s not found in tl-scheme\", target_f_name.c_str());" << NL
            << "return {};" << NL
            << END << NL;
          W << "const auto &storer_kv = tl_storers_ht.get_value(target_f_name);" << NL;
          W << "result_fetcher->" << combinator->get_var_num_arg(as_type_var->var_num)->name << ".fetcher = storer_kv(_cur_arg);" << NL;
        } else {
          W << "if (tl_object->$" << arg->name << ".is_null()) " << BEGIN
            << R"(CurrentProcessingQuery::get().raise_storing_error("Field \")" << arg->name << R"(\" not found in tl object");)" << NL
            << "return {};" << NL
            << END << NL;
          W << "result_fetcher->" << combinator->get_var_num_arg(as_type_var->var_num)->name << ".fetcher = "
            << get_tl_object_field_access(arg, field_rw_type::READ) << ".get()->store();" << NL;
        }
      } else if (arg->var_num != -1 && type_of(arg->type_expr)->is_integer_variable()) {
        // Запоминаем филд маску для последующего использования
        // Может быть либо локальной переменной либо полем структуры
        if (!typed_mode) {
          W << fmt_format("{}{} = tl_arr_get(tl_object, {}, {}, {}).to_int();",
                      var_num_access,
                      combinator->get_var_num_arg(arg->var_num)->name,
                      register_tl_const_str(arg->name),
                      arg->idx,
                      hash_tl_const_str(arg->name)) << NL;
        } else {
          W << var_num_access << combinator->get_var_num_arg(arg->var_num)->name << " = " << get_tl_object_field_access(arg, field_rw_type::READ) << ";" << NL;
        }
      }
      if (arg->is_fields_mask_optional()) {
        W << END << NL;
      }
    }
  }
private:
  std::string get_value_absence_check_for_optional_arg(const vk::tl::arg *arg) const {
    kphp_assert(arg->is_fields_mask_optional());
    auto type = type_of(arg->type_expr);
    std::string check_target = "tl_object->$" + arg->name;
    if (is_tl_type_wrapped_to_OrFalse(type_of(arg->type_expr))) {
      // Если оборачивается в OrFalse под филд маской
      return "!" + check_target + ".bool_value";
    } else if (type->id == TL_LONG) {
      // Если это var (mixed)
      return "equals(" + check_target + ", false)";
    } else if (!CUSTOM_IMPL_TYPES.count(type->name)) {
      // Если это class_instance
      return check_target + ".is_null()";
    } else {
      // Иначе это bool или OrFalse, которые не оборачиваются в OrFalse под филд маской,
      // поэтому мы не можем проверить записал ли разработчик значение
      return "";
    }
  }
};

std::string get_fetcher_call(const std::unique_ptr<vk::tl::type_expr_base> &type_expr, bool typed_mode = false, const std::string &storage = "") {
  if (!typed_mode) {
    return get_full_value(type_expr.get(), "") + ".fetch()";
  } else {
    kphp_assert(!storage.empty());
    return get_full_value(type_expr.get(), "") + ".typed_fetch_to(" + storage + ")";
  }
}

// Структура для генерации fetch любого type expression'а
struct TypeExprFetch {
  const std::unique_ptr<vk::tl::arg> &arg;
  bool typed_mode;

  explicit inline TypeExprFetch(const std::unique_ptr<vk::tl::arg> &arg, bool typed_mode = false) :
    arg(arg),
    typed_mode(typed_mode) {}

  inline void compile(CodeGenerator &W) const {
    const auto magic_fetching = get_magic_fetching(arg->type_expr.get(),
                                                   fmt_format("Incorrect magic of arg: {}\\nin constructor: {}", arg->name, cur_combinator->name));
    if (!magic_fetching.empty()) {
      W << magic_fetching << NL;
    }
    if (!typed_mode) {
      W << "result.set_value(" << register_tl_const_str(arg->name) << ", " << get_fetcher_call(arg->type_expr) << ", "
        << hash_tl_const_str(arg->name)
        << ");" << NL;
    } else {
      W << get_fetcher_call(arg->type_expr, true, get_tl_object_field_access(arg, field_rw_type::WRITE)) << ";" << NL;
    }
  }
};

/* Общий код для генерации fetch(...) для комбинаторов (то есть функций или контсрукторов).
 * Содержит основную логику.
 * Есть несколько частей:
 * 1) Обработка филд масок:
    array<var> c_hints_objectExt::fetch(int fields_mask) {
      array<var> result;
      result.set_value(tl_str$type, t_Int().fetch(), -445613708);
      result.set_value(tl_str$object_id, t_Int().fetch(), 65801733);
      if (fields_mask & (1 << 0)) {
        result.set_value(tl_str$rating, t_Double().fetch(), -1130913585);
      }
      if (fields_mask & (1 << 1)) {
        result.set_value(tl_str$text, t_String().fetch(), -193436300);
      }
      return result;
    }
 * 2) Обработка восклицательных знаков:
    var f_rpcProxy_diagonalTargets::fetch() {
      fetch_magic_if_not_bare(0x1cb5c415, "Incorrect magic in result of function: rpcProxy.diagonalTargets");
      return t_Vector<t_Vector<t_Maybe<tl_exclamation_fetch_wrapper, 0>, 0>, 0>(t_Vector<t_Maybe<tl_exclamation_fetch_wrapper, 0>, 0>(t_Maybe<tl_exclamation_fetch_wrapper, 0>(std::move(X)))).fetch();
    }
 * 3) Обработка основной части типового выражения в TypeExprStore / Fetch
*/
struct CombinatorFetch {
  const vk::tl::combinator *combinator;
  bool typed_mode;

  explicit CombinatorFetch(const vk::tl::combinator *combinator, bool typed_mode = false) :
    combinator(combinator),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const {
    cur_combinator = combinator;
    // Создаем локальные переменные для хранения филд масок
    if (combinator->is_constructor()) {
      auto fields_masks = get_not_optional_fields_masks(combinator);
      for (const auto &item : fields_masks) {
        W << "int " << item << "{0}; (void) " << item << ";" << NL;
      }
    }
    // fetch для функций - fetch возвращаемого значения, в отличии от fetch конструкторов
    if (combinator->is_function()) {
      // Если функция возвращает флатящийся тип, то после perform_flat_optimization() вместо него подставляется его внутренность.
      // Но при фетчинге функции нам всегда нужен мэджик оригинального типа, даже если он флатится.
      // Для этого было введено поле original_result_constructor_id у функций, в котором хранится мэджик реального типа, если результат флатится, и 0 иначе.
      if (!combinator->original_result_constructor_id) {
        const auto &magic_fetching = get_magic_fetching(combinator->result.get(),
                                                        fmt_format("Incorrect magic in result of function: {}", cur_combinator->name));
        if (!magic_fetching.empty()) {
          W << magic_fetching << NL;
        }
      } else {
        W << fmt_format(R"(fetch_magic_if_not_bare({:#010x}, "Incorrect magic in result of function: {}");)",
                        static_cast<uint32_t>(combinator->original_result_constructor_id), cur_combinator->name)
          << NL;
      }
      if (!typed_mode) {
        W << "return " << get_fetcher_call(combinator->result) << ";" << NL;
      } else {
        if (auto type_var = combinator->result->as<vk::tl::type_var>()) {
          // multiexclamation оптимизация
          W << "return " << cur_combinator->get_var_num_arg(type_var->var_num)->name << ".fetcher->typed_fetch();" << NL;
        } else {
          // для любой getChatInfo implements RpcFunction есть getChatInfo_result implements RpcFunctionReturnResult
          W << get_php_runtime_type(combinator, true, combinator->name + "_result") << " result;" << NL
            << "result.alloc();" << NL
            << get_fetcher_call(combinator->result, true, "result->$value") << ";" << NL
            << "return result;" << NL;
        }
      }
      return;
    }
    if (!typed_mode) {
      W << "array<var> result;" << NL;
    }
    for (const auto &arg : combinator->args) {
      // Если аргумент является параметром типа (обрамлен в {}), пропускаем его
      // Все такие аргументы хранятся в типе, как поля структуры
      if (arg->is_optional()) {
        continue;
      }
      // Если поле необязательное и зависит от филд маски
      if (arg->is_fields_mask_optional()) {
        W << fmt_format("if ({} & (1 << {})) ",
                        combinator->get_var_num_arg(arg->exist_var_num)->name,
                        arg->exist_var_bit) << BEGIN;
      }
      W << TypeExprFetch(arg, typed_mode);
      if (arg->var_num != -1 && type_of(arg->type_expr)->is_integer_variable()) {
        // запоминаем филд маску для дальнейшего использования
        if (!typed_mode) {
          W << combinator->get_var_num_arg(arg->var_num)->name << " = result.get_value(" << register_tl_const_str(arg->name) << ", "
            << hash_tl_const_str(arg->name) << ").to_int();" << NL;
        } else {
          W << combinator->get_var_num_arg(arg->var_num)->name << " = " << get_tl_object_field_access(arg, field_rw_type::READ) << ";" << NL;
        }
      }
      if (arg->is_fields_mask_optional()) {
        W << END << NL;
      }
    }
    if (!typed_mode) {
      W << "return result;" << NL;
    }
  }
};

struct TlTemplatePhpTypeHelpers {
  explicit TlTemplatePhpTypeHelpers(const vk::tl::type *type) :
    type(get_this_from_renamed_tl_scheme(type)),
    constructor(nullptr) {}

  explicit TlTemplatePhpTypeHelpers(const vk::tl::combinator *constructor) :
    type(nullptr),
    constructor(get_this_from_renamed_tl_scheme(constructor)) {}

  inline void compile(CodeGenerator &W) const {
    int cnt = 0;
    std::vector<std::string> type_var_names;
    const vk::tl::combinator *target_constructor = type ? type->constructors[0].get() : constructor;
    for (const auto &arg : target_constructor->args) {
      if (arg->is_type(tl)) {
        ++cnt;
        type_var_names.emplace_back(arg->name);
      }
    }
    const std::string &struct_name = cpp_tl_struct_name("", type ? type->name : constructor->name, "__");
    W << "template <" << vk::join(std::vector<std::string>(cnt, "typename"), ", ") << ">" << NL;
    W << "struct " << struct_name << " " << BEGIN
      << "using type = tl_undefined_php_type;" << NL
      << END << ";" << NL << NL;
    auto php_classes = type ? get_all_php_classes_of_tl_type(type) : get_all_php_classes_of_tl_constructor(constructor);
    for (const auto &cur_php_class_template_instantiation : php_classes) {
      const std::string &cur_instantiation_name = cur_php_class_template_instantiation->src_name;
      std::vector<std::string> template_specialization_params = type_var_names;
      std::string type_var_access;
      if (type) {
        type_var_access = cur_instantiation_name;
      } else {
        kphp_assert(constructor);
        if (tl->get_type_by_magic(constructor->type_id)->name == "ReqResult") {
          type_var_access = "C$VK$TL$RpcResponse";
        } else {
          int pos = cur_instantiation_name.find("__");
          kphp_assert(pos != std::string::npos);
          ClassPtr tl_type_php_class = get_php_class_of_tl_type_specialization(tl->get_type_by_magic(constructor->type_id), cur_instantiation_name.substr(pos));
          kphp_assert(tl_type_php_class);
          type_var_access = tl_type_php_class->src_name;
        }
      }
      std::transform(template_specialization_params.begin(), template_specialization_params.end(), template_specialization_params.begin(),
                     [&](const std::string &s) { return type_var_access + "::" += s; });
      W << "template <>" << NL;
      W << "struct " << struct_name << "<" << vk::join(template_specialization_params, ", ") << "> " << BEGIN;
      W << "using type = " << cur_instantiation_name << ";" << NL;
      W << END << ";" << NL << NL;
    }
  }
private:
  const vk::tl::type *type;
  const vk::tl::combinator *constructor;
};

struct TlConstructorDecl {
  const vk::tl::combinator *constructor;

  static bool does_tl_constructor_need_typed_fetch_store(const vk::tl::combinator *c) {
    return !get_all_php_classes_of_tl_constructor(c).empty();
  }

  static std::string get_optional_args_for_decl(const vk::tl::combinator *c) {
    std::vector<std::string> res;
    for (const auto &arg : c->args) {
      if (arg->is_optional()) {
        if (type_of(arg->type_expr)->is_integer_variable()) {
          res.emplace_back("int " + arg->name);
        } else {
          res.emplace_back("T" + std::to_string(arg->var_num) + " " + arg->name);
        }
      }
    }
    return vk::join(res, ", ");
  }

  explicit TlConstructorDecl(const vk::tl::combinator *constructor) :
    constructor(constructor) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor);

    if (needs_typed_fetch_store && is_type_dependent(constructor, tl)) {
      W << TlTemplatePhpTypeHelpers(constructor);
    }

    std::string template_decl = get_template_declaration(constructor);
    if (!template_decl.empty()) {
      W << template_decl << NL;
    }
    W << "struct " << cpp_tl_struct_name("c_", constructor->name) << " " << BEGIN;
    auto params = get_optional_args_for_decl(constructor);
    W << "static void store(const var &tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
    W << "static array<var> fetch(" << params << ");" << NL;

    if (needs_typed_fetch_store) {
      std::string php_type = get_php_runtime_type(constructor, false);
      W << "static void typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
      W << "static void typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
    }
    W << END << ";\n\n";
  }
};

struct TlConstructorDef {
  const vk::tl::combinator *constructor;

  explicit TlConstructorDef(const vk::tl::combinator *constructor) :
    constructor(constructor) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor);

    std::string template_decl = get_template_declaration(constructor);
    std::string template_def = get_template_definition(constructor);
    auto full_struct_name = cpp_tl_struct_name("c_", constructor->name, template_def);
    auto params = TlConstructorDecl::get_optional_args_for_decl(constructor);

    W << template_decl << NL;
    W << "void " << full_struct_name + "::store(const var& tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
    W << CombinatorStore(constructor);
    W << END << "\n\n";

    W << template_decl << NL;
    W << "array<var> " << full_struct_name + "::fetch(" << params << ") " << BEGIN;
    W << CombinatorFetch(constructor);
    W << END << "\n\n";

    if (needs_typed_fetch_store) {
      std::string php_type = get_php_runtime_type(constructor, false);
      W << template_decl << NL;
      W << "void " << full_struct_name + "::typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
      W << CombinatorStore(constructor, true);
      W << END << "\n\n";

      W << template_decl << NL;
      W << "void " << full_struct_name << "::typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
      W << CombinatorFetch(constructor, true);
      W << END << "\n\n";
    }
  }
};
/*
 * Пример типизированного:
template <typename, typename>
struct Either__ {
  using type = tl_undefined_php_type;
};

template <>
struct Either__<C$VK$TL$Types$Either__string__graph_Vertex::X, C$VK$TL$Types$Either__string__graph_Vertex::Y> {
  using type = C$VK$TL$Types$Either__string__graph_Vertex;
};

template<typename T0, unsigned int inner_magic0, typename T1, unsigned int inner_magic1>
struct t_Either {
  using PhpType = class_instance<typename Either__<typename T0::PhpType, typename T1::PhpType>::type>;
  T0 X;
  T1 Y;
  explicit t_Either(T0 X, T1 Y) : X(std::move(X)), Y(std::move(Y)) {}

  void store(const var& tl_object);
  array<var> fetch();
  void typed_store(const PhpType &tl_object);
  void typed_fetch_to(PhpType &tl_object);
};
*/
struct TlTypeDecl {
  const vk::tl::type *t;

  // tl-типу 'messages.ChatInfoUser' соответствует:
  // * либо php class VK\TL\Types\messages\chatInfoUser
  // * либо php interface VK\TL\Types\messages\ChatInfoUser, если тип полиморфный
  static bool does_tl_type_need_typed_fetch_store(const vk::tl::type *t) {
    if (t->name == "ReqResult") {
      // без этого сайт не компилится, пока typed rpc не подключен — т.к. типизированный t_ReqResult не компилится
      bool typed_php_code_exists = !!G->get_class(vk::tl::PhpClasses::rpc_response_ok_with_tl_full_namespace());
      return typed_php_code_exists;
    }
    return !get_all_php_classes_of_tl_type(t).empty();
  }

  explicit TlTypeDecl(const vk::tl::type *t) :
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << t->name << " ~~~~~~~~~ */\n" << NL;
    const bool needs_typed_fetch_store = TlTypeDecl::does_tl_type_need_typed_fetch_store(t);
    if (needs_typed_fetch_store && is_type_dependent(t, tl) && t->is_polymorphic()) {
      // Если тип не полиморфный, то пользуемся type helper'ами конструктора, которые точно должны быть сгенерены и доступны
      W << TlTemplatePhpTypeHelpers(t);
    }
    std::string struct_name = cpp_tl_struct_name("t_", t->name);
    auto constructor = t->constructors[0].get();
    std::string template_decl = get_template_declaration(constructor);
    if (!template_decl.empty()) {
      W << template_decl << NL;
    }
    W << "struct " << struct_name << " " << BEGIN;
    W << "using PhpType = "
      << (needs_typed_fetch_store ? get_php_runtime_type(t) : "tl_undefined_php_type") << ";" << NL;
    std::vector<std::string> constructor_params;
    std::vector<std::string> constructor_inits;
    for (const auto &arg : constructor->args) {
      if (arg->var_num != -1) {
        if (type_of(arg->type_expr)->is_integer_variable()) {
          if (arg->is_optional()) {
            W << "int " << arg->name << "{0};" << NL;
            constructor_params.emplace_back("int " + arg->name);
            constructor_inits.emplace_back(fmt_format("{}({})", arg->name, arg->name));
          }
        } else if (type_of(arg->type_expr)->name == T_TYPE) {
          W << fmt_format("T{} {};", arg->var_num, arg->name) << NL;
          kphp_assert(arg->is_optional());
          constructor_params.emplace_back(fmt_format("T{} {}", arg->var_num, arg->name));
          constructor_inits.emplace_back(fmt_format("{}(std::move({}))", arg->name, arg->name));
        }
      }
    }
    if (!constructor_params.empty()) {
      W << "explicit " << struct_name << "(" << vk::join(constructor_params, ", ") << ") : " << vk::join(constructor_inits, ", ") << " {}\n" << NL;
    }
    W << "void store(const var& tl_object);" << NL;
    W << "array<var> fetch();" << NL;
    if (needs_typed_fetch_store) {
      W << "void typed_store(const PhpType &tl_object);" << NL;
      W << "void typed_fetch_to(PhpType &tl_object);" << NL;
    }
    W << END << ";\n\n";
  }
};

struct TlTypeDef {
  const vk::tl::type *t;

  explicit TlTypeDef(const vk::tl::type *t) :
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlTypeDecl::does_tl_type_need_typed_fetch_store(t);

    auto constructor = t->constructors[0].get();
    std::string struct_name = cpp_tl_struct_name("t_", t->name);
    std::string template_decl = get_template_declaration(constructor);
    std::string template_def = get_template_definition(constructor);
    auto full_struct_name = struct_name + template_def;

    W << template_decl << NL;
    W << "void " << full_struct_name << "::store(const var &tl_object) " << BEGIN;
    W << TypeStore(t, template_def);
    W << END << "\n\n";

    W << template_decl << NL;
    W << "array<var> " << full_struct_name + "::fetch() " << BEGIN;
    W << TypeFetch(t, template_def);
    W << END << "\n\n";

    if (needs_typed_fetch_store) {
      W << template_decl << NL;
      W << "void " << full_struct_name << "::typed_store(const PhpType &tl_object) " << BEGIN;
      W << TypeStore(t, template_def, true);
      W << END << "\n\n";

      W << template_decl << NL;
      W << "void " << full_struct_name + "::typed_fetch_to(PhpType &tl_object) " << BEGIN;
      W << TypeFetch(t, template_def, true);
      W << END << "\n\n";
    }
  }
};

struct TlFunctionDecl {
  const vk::tl::combinator *f;

  static bool does_tl_function_need_typed_fetch_store(const vk::tl::combinator *f) {
    return !!get_php_class_of_tl_function(f);
  }

  explicit TlFunctionDecl(const vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << f->name << " ~~~~~~~~~ */\n" << NL;
    const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);

    std::string struct_name = cpp_tl_struct_name("f_", f->name);
    W << "struct " + struct_name + " : tl_func_base " << BEGIN;
    for (const auto &arg : f->args) {
      if (arg->is_optional()) {
        W << "tl_exclamation_fetch_wrapper " << arg->name << ";" << NL;
      } else if (arg->var_num != -1) {
        W << "int " << arg->name << "{0};" << NL;
      }
    }
    W << "static std::unique_ptr<tl_func_base> store(const var &tl_object);" << NL;
    W << "var fetch();" << NL;
    if (needs_typed_fetch_store) {
      W << "static std::unique_ptr<tl_func_base> typed_store(const " << get_php_runtime_type(f) << " *tl_object);" << NL;
      W << "class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> typed_fetch();" << NL;
    }
    W << END << ";" << NL << NL;
  }
};

class TlFunctionDef {
  const vk::tl::combinator *f;

public:
  explicit TlFunctionDef(const vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);
    std::string struct_name = cpp_tl_struct_name("f_", f->name);

    W << "std::unique_ptr<tl_func_base> " << struct_name << "::store(const var& tl_object) " << BEGIN;
    W << "auto result_fetcher = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorStore(f);
    W << "return std::move(result_fetcher);" << NL;
    W << END << NL << NL;

    W << "var " << struct_name << "::fetch() " << BEGIN;
    W << CombinatorFetch(f);
    W << END << NL << NL;

    if (needs_typed_fetch_store) {
      W << "std::unique_ptr<tl_func_base> " << struct_name << "::typed_store(const " << get_php_runtime_type(f) << " *tl_object) " << BEGIN;
      W << "auto result_fetcher = make_unique_on_script_memory<" << struct_name << ">();" << NL;
      W << CombinatorStore(f, true);
      W << "return std::move(result_fetcher);" << NL;
      W << END << NL << NL;

      W << "class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> " << struct_name << "::typed_fetch() " << BEGIN;
      W << CombinatorFetch(f, true);
      W << END << NL << NL;
    }
  }
};

class Module;

extern std::unordered_map<std::string, Module> modules;

std::set<std::string> modules_with_functions;

// Модуль - это два .cpp/.h файла, соответстсвующие одному модулю tl-схемы
class Module {
public:
  std::vector<vk::tl::type *> target_types;
  std::vector<vk::tl::combinator *> target_functions;
  IncludesCollector h_includes;
  IncludesCollector cpp_includes;
  std::string name;

  Module() = default;

  explicit Module(string name) :
    name(std::move(name)) {}

  void compile_tl_h_file(CodeGenerator &W) const {
    W << OpenFile(name + ".h", "tl");
    W << "#pragma once" << NL;
    W << ExternInclude("runtime/tl/tl_builtins.h");
    W << ExternInclude("tl/tl_const_vars.h");
    W << h_includes;

    W << NL;

    for (const auto &t : target_types) {
      for (const auto &constructor : t->constructors) {
        W << TlConstructorDecl(constructor.get());
      }
    }
    for (const auto &t : target_types) {
      W << TlTypeDecl(t);
      if (is_type_dependent(t, tl)) {
        W << TlTypeDef(t);
      }
    }
    for (const auto &t : target_types) {
      if (is_type_dependent(t, tl)) {
        for (const auto &constructor : t->constructors) {
          W << TlConstructorDef(constructor.get());
        }
      }
    }
    for (const auto &f : target_functions) {
      W << TlFunctionDecl(f);
    }
    W << CloseFile();
  }

  void compile_tl_cpp_file(CodeGenerator &W) const {
    W << OpenFile(name + ".cpp", "tl", false);
    W << ExternInclude("php_functions.h");
    W << Include("tl/" + name + ".h") << NL;
    W << cpp_includes;

    for (const auto &t : target_types) {
      if (!is_type_dependent(t, tl)) {
        W << TlTypeDef(t);
        for (const auto &constructor : t->constructors) {
          W << TlConstructorDef(constructor.get());
        }
      }
    }
    for (const auto &f: target_functions) {
      W << TlFunctionDef(f);
    }
    W << CloseFile();
  }

  void compile(CodeGenerator &W) const {
    compile_tl_h_file(W);
    compile_tl_cpp_file(W);
  }

  void add_obj(const std::unique_ptr<vk::tl::combinator> &f) {
    modules_with_functions.insert(name);
    target_functions.push_back(f.get());
    update_dependencies(f);

    if (TlFunctionDecl::does_tl_function_need_typed_fetch_store(f.get())) {
      ClassPtr klass = get_php_class_of_tl_function(f.get());
      kphp_assert(klass);
      h_includes.add_class_forward_declaration(klass);
      cpp_includes.add_class_include(klass);
      klass = get_php_class_of_tl_function_result(f.get());
      kphp_assert(klass);
      cpp_includes.add_class_include(klass);
    }
  }

  void add_obj(const std::unique_ptr<vk::tl::type> &t) {
    target_types.push_back(t.get());
    update_dependencies(t);

    for (const auto &c : t->constructors) {
      if (TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(c.get())) {
        auto php_classes = get_all_php_classes_of_tl_constructor(c.get());
        std::for_each(php_classes.begin(), php_classes.end(), [&](ClassPtr klass) { h_includes.add_class_include(klass); });
      }
    }
  }

  static std::string get_module_name(const std::string &type_or_comb_name) {
    auto pos = type_or_comb_name.find('.');
    return pos == std::string::npos ? "common" : type_or_comb_name.substr(0, pos);
  }

  static void add_to_module(const std::unique_ptr<vk::tl::type> &t) {
    ensure_existence(get_module_name(t->name)).add_obj(t);
  }

  static void add_to_module(const std::unique_ptr<vk::tl::combinator> &c) {
    ensure_existence(get_module_name(c->name)).add_obj(c);
  }

private:
  static Module &ensure_existence(const std::string &module_name) {
    if (modules.find(module_name) == modules.end()) {
      modules[module_name] = Module(module_name);
    }
    return modules[module_name];
  }

  void update_dependencies(const std::unique_ptr<vk::tl::combinator> &combinator) {
    for (const auto &arg : combinator->args) {
      collect_deps_from_type_tree(arg->type_expr.get());
    }
    if (combinator->is_function()) {
      collect_deps_from_type_tree(combinator->result.get());
    }
  }

  void update_dependencies(const std::unique_ptr<vk::tl::type> &t) {
    std::string type_module = get_module_name(t->name);
    for (const auto &c : t->constructors) {
      update_dependencies(c);
      if (type_module != name) {
        ensure_existence(type_module).h_includes.add_raw_filename_include("tl/" + name + ".h");
      }
    }
  }

  void collect_deps_from_type_tree(vk::tl::expr_base *expr) {
    if (auto as_type_expr = expr->as<vk::tl::type_expr>()) {
      std::string expr_module = get_module_name(tl->types[as_type_expr->type_id]->name);
      if (name != expr_module) {
        h_includes.add_raw_filename_include("tl/" + expr_module + ".h");
      }
      for (const auto &child : as_type_expr->children) {
        collect_deps_from_type_tree(child.get());
      }
    } else if (auto as_type_array = expr->as<vk::tl::type_array>()) {
      for (const auto &arg : as_type_array->args) {
        collect_deps_from_type_tree(arg->type_expr.get());
      }
    }
  }
};

std::unordered_map<std::string, Module> modules;

//Рекурсивно обходим дерево выражения типа и возвращаем пару <тип, значение>
std::pair<std::string, std::string> get_full_type_expr_str(vk::tl::expr_base *type_expr, const std::string &var_num_access) {
  if (auto as_nat_var = type_expr->as<vk::tl::nat_var>()) {
    return {"", var_num_access + cur_combinator->get_var_num_arg(as_nat_var->var_num)->name};
  }
  if (auto as_nat_const = type_expr->as<vk::tl::nat_const>()) {
    return {"", std::to_string(as_nat_const->num)};
  }
  if (auto as_type_var = type_expr->as<vk::tl::type_var>()) {
    std::string expr = "std::move(" + cur_combinator->get_var_num_arg(as_type_var->var_num)->name + ")";
    if (cur_combinator->is_constructor()) {
      return {"T" + std::to_string(as_type_var->var_num), expr};
    } else {
      return {"tl_exclamation_fetch_wrapper", expr};
    }
  }
  if (auto as_type_array = type_expr->as<vk::tl::type_array>()) {
    std::string inner_magic = "0";
    // После replace_anonymous_args содержимое ячеек тл массивов, в которых несколько аргументов, выносится в отдельный тип и подставляется единственным аргументом
    kphp_assert(as_type_array->args.size() == 1);
    if (auto casted = as_type_array->args[0]->type_expr->as<vk::tl::type_expr>()) {
      if (is_magic_processing_needed(casted)) {
        inner_magic = fmt_format("{:#010x}", static_cast<unsigned int>(type_of(casted)->id));
      }
    } else {
      kphp_error(false, "Too complicated tl array");
    }
    std::string array_item_type_name = cpp_tl_struct_name("t_", type_of(as_type_array->args[0]->type_expr)->name);
    std::string type = fmt_format("tl_array<{}, {}>", array_item_type_name, inner_magic);
    return {type, type + fmt_format("({}, {}())",
                                    get_full_value(as_type_array->multiplicity.get(), var_num_access),
                                    array_item_type_name)};
  }
  auto as_type_expr = type_expr->as<vk::tl::type_expr>();
  kphp_assert(as_type_expr);
  const auto &type = tl->types[as_type_expr->type_id];
  std::vector<std::string> template_params;
  std::vector<std::string> constructor_params;
  for (const auto &child : as_type_expr->children) {
    auto child_type_str = get_full_type_expr_str(child.get(), var_num_access);
    std::string child_type_name = child_type_str.first;
    std::string child_type_value = child_type_str.second;
    constructor_params.emplace_back(child_type_value);
    if (!child_type_name.empty()) {
      kphp_assert(child->as<vk::tl::type_expr_base>());
      template_params.emplace_back(child_type_name);
      if (auto child_as_type_expr = child->as<vk::tl::type_expr>()) {
        if (is_magic_processing_needed(child_as_type_expr)) {
          template_params.emplace_back(fmt_format("{:#010x}", static_cast<unsigned int>(type_of(child_as_type_expr)->id)));
        } else {
          template_params.emplace_back("0");
        }
      } else if (auto child_as_type_var = child->as<vk::tl::type_var>()) {
        if (vk::string_view(child_type_name).starts_with("T")) {
          template_params.emplace_back(fmt_format("inner_magic{}", child_as_type_var->var_num));
        } else {
          template_params.emplace_back("0");
        }
      } else {
        kphp_assert(child->as<vk::tl::type_array>());
        template_params.emplace_back("0");
      }
    }
  }
  auto type_name = (!type->is_integer_variable() ? cpp_tl_struct_name("t_", type->name) : cpp_tl_struct_name("t_", tl->types[TL_INT]->name));
  auto template_str = (!template_params.empty() ? "<" + vk::join(template_params, ", ") + ">" : "");
  auto full_type_name = type_name + template_str;
  auto full_type_value = full_type_name + "(" + vk::join(constructor_params, ", ") + ")";
  return {full_type_name, full_type_value};
}

// сейчас тут проверка следующая: если тип полиморфный, то он нигде не должен использоваться как bare
// по идее, этой проверки тут БЫТЬ НЕ ДОЛЖНО: она должна быть на уровне генерации tlo; но пока тут
// более того, она пока что ОТКЛЮЧЕНА: включить, когда починим tree_stats.CounterChangeRequestManualLimit и storage2.UploadBlockSliceLink
void check_type_expr(vk::tl::expr_base *expr_base) {
  if (false) {
    if (auto array = expr_base->as<vk::tl::type_array>()) {
      for (const auto &arg : array->args) {
        check_type_expr(arg->type_expr.get());
      }
    } else if (auto type_expr = expr_base->as<vk::tl::type_expr>()) {
      vk::tl::type *type = type_of(type_expr);
      if (type->is_integer_variable() || type->name == T_TYPE) {
        return;
      }
      kphp_error(!type->is_polymorphic() || !type_expr->is_bare(),
                 fmt_format("Polymorphic tl type {} can't be used as bare in tl scheme.", type->name));
      for (const auto &child : type_expr->children) {
        check_type_expr(child.get());
      }
    }
  }
}

void check_combinator(const std::unique_ptr<vk::tl::combinator> &c) {
  for (const auto &arg : c->args) {
    check_type_expr(arg->type_expr.get());
  }
}

void check_constructor(const std::unique_ptr<vk::tl::combinator> &c) {
  check_combinator(c);
  // Проверяем, что порядок неявных аргументов конструктора совпадает с их порядком в типе
  std::vector<int> var_nums;
  for (const auto &arg : c->args) {
    if (arg->is_optional() && arg->var_num != -1) {
      var_nums.push_back(arg->var_num);
    }
  }
  std::vector<int> params_order;
  auto as_type_expr = c->result->as<vk::tl::type_expr>();
  int idx = 0;
  for (const auto &child : as_type_expr->children) {
    if (auto as_nat_var = child->as<vk::tl::nat_var>()) {
      params_order.push_back(as_nat_var->var_num);
    } else if (auto as_type_var = child->as<vk::tl::type_var>()) {
      params_order.push_back(as_type_var->var_num);
    }
    idx += 1;
  }
  kphp_error(var_nums == params_order,
             fmt_format("Strange tl scheme here: {}", c->name));
  // Также проверяем, что индексы аргументов типовых переменных в конструкторе совпадают с порядковым (слева направо, начиная с 0)
  // номером соответстсвующей вершины-ребенка в типовом дереве
  std::vector<int> indices_in_constructor;
  for (int i = 0; i < c->args.size(); ++i) {
    if (c->args[i]->is_type(tl)) {
      indices_in_constructor.push_back(i);
    }
  }
  std::vector<int> indices_in_type_tree;
  int i = 0;
  for (const auto &child : as_type_expr->children) {
    if (dynamic_cast<vk::tl::type_var *>(child.get())) {
      indices_in_type_tree.push_back(i);
    }
    ++i;
  }
  kphp_error(indices_in_constructor == indices_in_type_tree,
             fmt_format("Strange tl scheme here: {}", c->name));
}

/* Разбиваем все комбинаторы и типы на модули, вместе с тем собирая зависимости каждого модуля.
 * */
void collect_target_objects() {
  auto should_exclude_tl_type = [](const std::unique_ptr<vk::tl::type> &t) {
    return CUSTOM_IMPL_TYPES.find(t->name) != CUSTOM_IMPL_TYPES.end()
           || t->name == "engine.Query";                        // это единственный тип с !X, надо бы его удалить из схемы
  };

  for (const auto &e : tl->types) {
    const std::unique_ptr<vk::tl::type> &t = e.second;
    if (!should_exclude_tl_type(t)) {
      Module::add_to_module(t);
      for (const auto &c : t->constructors) {
        cur_combinator = c.get();
        check_constructor(c);
      }
    }
  }

  for (const auto &e : tl->functions) {
    const std::unique_ptr<vk::tl::combinator> &f = e.second;
    Module::add_to_module(f);
    cur_combinator = f.get();
    check_combinator(f);
  }
}

void write_tl_query_handlers(CodeGenerator &W) {
  if (G->env().get_tl_schema_file().empty()) {
    return;
  }

  auto tl_ptr = vk::tl::parse_tlo(G->env().get_tl_schema_file().c_str(), false);
  kphp_error_return(tl_ptr.has_value(),
                    fmt_format("Error while reading tlo: {}", tl_ptr.error()));

  tl = tl_ptr.value().get();
  replace_anonymous_args(*tl);
  perform_flat_optimization(*tl);
  collect_target_objects();
  for (const auto &e : modules) {
    const Module &module = e.second;
    W << module;
  }

  W << OpenFile("tl_store_funs_table.cpp", "tl", false);
  for (const auto &module_name : modules_with_functions) {
    W << Include("tl/" + module_name + ".h");
  }
  W << NL;
  // Указатель на gen$tl_fetch_wrapper прокидывается в рантайм и вызывается из fetch_function()
  // Это сделано для того, чтобы не тащить в рантайм t_ReqResult и все его зависимости
  W << "array<var> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base> stored_fetcher) " << BEGIN
    << "tl_exclamation_fetch_wrapper X(std::move(stored_fetcher));" << NL
    << "return t_ReqResult<tl_exclamation_fetch_wrapper, 0>(std::move(X)).fetch();" << NL
    << END << NL << NL;
  // Хэш таблица, содержащая все тл функции
  // Тоже прокидывается в рантайм
  W << "array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
  W << "void fill_tl_storers_ht() " << BEGIN;
  for (const auto &module_name : modules_with_functions) {
    for (const auto &f : modules[module_name].target_functions) {
      W << "gen$tl_storers_ht.set_value(" << register_tl_const_str(f->name) << ", " << "&" << cpp_tl_struct_name("f_", f->name) << "::store, "
        << hash_tl_const_str(f->name) << ");" << NL;
    }
  }
  W << END << NL;
  W << CloseFile();

  // Вынесенные строковые константы
  W << OpenFile("tl_const_vars.h", "tl");
  W << "#pragma once" << NL;
  W << ExternInclude("php_functions.h");
  std::for_each(tl_const_vars.begin(), tl_const_vars.end(), [&W](const std::string &s) {
    W << "extern string " << cpp_tl_const_str(s) << ";" << NL;
  });
  W << CloseFile();

  W << OpenFile("tl_const_vars.cpp", "tl", false);
  W << ExternInclude("php_functions.h");
  W << Include("tl/tl_const_vars.h");
  std::for_each(tl_const_vars.begin(), tl_const_vars.end(), [&W](const std::string &s) {
    W << "string " << cpp_tl_const_str(s) << ";" << NL;
  });
  auto const_string_shifts = compile_raw_data(W, tl_const_vars);
  W << "void tl_str_const_init() " << BEGIN;
  int i = 0;
  std::for_each(tl_const_vars.begin(), tl_const_vars.end(), [&](const std::string &s) {
    W << cpp_tl_const_str(s) << ".assign_raw (&raw[" << const_string_shifts[i++] << "]);" << NL;
  });
  W << END;
  W << CloseFile();
}
} // namespace tl_gen

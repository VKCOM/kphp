#include "compiler/code-gen/files/tl2cpp.h"

#include <string>

#include "common/algorithms/string-algorithms.h"
#include "common/tlo-parsing/flat-optimization.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"

/*
 * Что осталось для типизированного rpc
 * todo: multiexclamation optimization + проверить, что с vkext совпадает
 * todo: зависимые типы, сведение шаблонов через using -- (смотреть коммент в джире к KPHP-402)
 *       (вероятно, потребует модификации get_php_class_of_tl_constructor)
 */

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
 * Наименования начинаются с названия сущности, для которой генерится код (Function, Type, Constructor, Cell, Combinator, TypeExpr)
 * Структуры, с вызова compile() которых, всегда (почти всегда - еще есть Cell) начинается генерация:
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
 *
 * Также есть отдельная структура Cell, compile() которой генерирует определение структуры для store/fetch cell'а
 */
namespace tl_gen {
static const std::string T_TYPE = "Type";
static const std::string T_INTEGER_VARIABLE = "#";

using vk::tl::FLAG_OPT_FIELD;
using vk::tl::FLAG_OPT_VAR;
using vk::tl::FLAG_DEFAULT_CONSTRUCTOR;
using vk::tl::FLAG_NOCONS;
using vk::tl::FLAG_EXCL;
using vk::tl::FLAG_BARE;

static vk::tl::tl_scheme *tl;
static vk::tl::combinator *cur_combinator;

const std::unordered_set<std::string> CUSTOM_IMPL_TYPES   // из tl_builtins.h
  {T_INTEGER_VARIABLE, T_TYPE, "Int", "Long", "Double", "String", "Bool", "False", "Vector", "Maybe", "Tuple",
   "Dictionary", "DictionaryField", "IntKeyDictionary", "IntKeyDictionaryField", "LongKeyDictionary", "LongKeyDictionaryField"};


// tl-конструктору 'messages.chatInfoUser' соответствует php class VK\TL\Types\messages\chatInfoUser
// todo понять, что с зависимыми типами; прокомментить про достижимость
ClassPtr get_php_class_of_tl_constructor(vk::tl::combinator *c) {
  std::string php_class_name;
  switch (static_cast<unsigned int>(c->id)) {   // отдельно — 3 конструктора t_ReqResult
    case TL__:
      php_class_name = G->env().get_tl_namespace_prefix() + "Types\\rpcResponseOk";
      break;
    case TL_REQ_RESULT_HEADER:
      php_class_name = G->env().get_tl_namespace_prefix() + "Types\\rpcResponseHeader";
      break;
    case TL_REQ_ERROR:
      php_class_name = G->env().get_tl_namespace_prefix() + "Types\\rpcResponseError";
      break;
    default:
      php_class_name = G->env().get_tl_namespace_prefix() + "Types\\" + replace_characters(c->name, '.', '\\');
  }
  return G->get_class(php_class_name);
}

// tl-функции 'messages.getChatInfo' соответствует php class VK\TL\Functions\messages\getChatInfo
// если этот класс достижим компилятором — типизированный вариант для messages.getChatInfo нужен
// (а если нет, то эта tl-функция типизированно гарантированно не вызывается)
ClassPtr get_php_class_of_tl_function(vk::tl::combinator *f) {
  std::string php_class_name = G->env().get_tl_namespace_prefix() + "Functions\\" + replace_characters(f->name, '.', '\\');
  return G->get_class(php_class_name);
}

// и наоборот: классу VK\TL\Functions\messages\getChatInfo соответствует tl-функция messages.getChatInfo
std::string get_tl_function_of_php_class(ClassPtr klass) {
  kphp_assert(is_php_class_a_tl_function(klass));
  size_t pos = vk::string_view{klass->name}.find("Functions\\");
  std::string after_ns_functions = static_cast<std::string>(vk::string_view{klass->name}.substr(pos + 10));
  return replace_characters(after_ns_functions, '\\', '.');
}

// у tl-функции 'messages.getChatInfo' есть типизированный результат php class VK\TL\Functions\messages\getChatInfo_result
// по идее, достижима функция <=> достижим результат
ClassPtr get_php_class_of_tl_function_result(vk::tl::combinator *f) {
  std::string php_class_name = G->env().get_tl_namespace_prefix() + "Functions\\" + replace_characters(f->name, '.', '\\') + "_result";
  return G->get_class(php_class_name);
}

// классы VK\TL\Functions\* implements RpcFunction — это те, что соответствуют tl-функциям
bool is_php_class_a_tl_function(ClassPtr klass) {
  return klass->is_tl_class && klass->implements.size() == 1 && vk::string_view{klass->implements.front()->name}.ends_with("RpcFunction");
}

// классы VK\TL\Types\* — не интерфейсные — соответствуют конструкторам
// (или неполиморфмным типам, единственный конструктор которых заинлайнен)
bool is_php_class_a_tl_constructor(ClassPtr klass) {
  return klass->is_tl_class && klass->is_class() && klass->name.find("Types\\") != std::string::npos;
}

// классы VK\TL\Types\* — интерфейсы — это полиморфные типы, конструкторы которых классы implements его
bool is_php_class_a_tl_polymorphic_type(ClassPtr klass) {
  return klass->is_tl_class && klass->is_interface() && klass->name.find("Types\\") != std::string::npos;
}

// Вспомогательные функции для генерации
inline namespace {
vk::tl::type *type_of(const std::unique_ptr<vk::tl::type_expr_base> &type_expr) {
  if (auto casted = type_expr->template as<vk::tl::type_expr>()) {
    return tl->types[casted->type_id].get();
  }
  return nullptr;
}

vk::tl::type *type_of(const vk::tl::type_expr *type) {
  return tl->types[type->type_id].get();
}

bool is_type_dependent(vk::tl::combinator *constructor) {
  for (const auto &arg : constructor->args) {
    auto arg_type = type_of(arg->type_expr);
    if (arg_type && arg_type->name == "Type") {
      return true;
    }
  }
  return false;
}

bool is_type_dependent(vk::tl::type *type) {
  return is_type_dependent(type->constructors[0].get());
}

template<typename T>
bool is_magic_processing_needed(const T &type_expr) {
  const auto &type = tl->types[type_expr->type_id];
  return type->constructors_num == 1 && !(type_expr->flags & FLAG_BARE);
}

std::vector<std::string> get_not_optional_fields_masks(const vk::tl::combinator *constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_INTEGER_VARIABLE && !(arg->flags & FLAG_OPT_VAR)) {
      res.emplace_back(arg->name);
    }
  }
  return res;
}

std::vector<std::string> get_optional_args_for_call(const std::unique_ptr<vk::tl::combinator> &constructor) {
  std::vector<std::string> res;
  for (const auto &arg : constructor->args) {
    if (arg->flags & FLAG_OPT_VAR) {
      if (type_of(arg->type_expr)->name == T_INTEGER_VARIABLE) {
        res.emplace_back(arg->name);
      } else {
        res.emplace_back("std::move(" + arg->name + ")");
      }
    }
  }
  return res;
}

std::vector<std::string> get_template_params(const std::unique_ptr<vk::tl::combinator> &constructor) {
  std::vector<std::string> typenames;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_TYPE) {
      kphp_assert(arg->flags & FLAG_OPT_VAR);
      typenames.emplace_back(format("T%d", arg->var_num));
      typenames.emplace_back(format("inner_magic%d", arg->var_num));
    }
  }
  return typenames;
}

std::string get_template_declaration(const std::unique_ptr<vk::tl::combinator> &constructor) {
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

std::string get_template_definition(const std::unique_ptr<vk::tl::combinator> &constructor) {
  std::vector<std::string> typenames = get_template_params(constructor);
  if (typenames.empty()) {
    return "";
  }
  return "<" + vk::join(typenames, ", ") + ">";
}

std::string get_php_runtime_type(const vk::tl::combinator *c, bool wrap_to_class_instance = false, const std::string &type_name = "") {
  std::string res;
  switch (static_cast<unsigned int>(c->id)) {
    case TL__:
      res = G->env().get_tl_classname_prefix() + "Types$rpcResponseOk";
      break;
    case TL_REQ_RESULT_HEADER:
      res = G->env().get_tl_classname_prefix() + "Types$rpcResponseHeader";
      break;
    case TL_REQ_ERROR:
      res = G->env().get_tl_classname_prefix() + "Types$rpcResponseError";
      break;
    default:
      std::string name = type_name.empty() ? c->name : type_name;
      std::replace(name.begin(), name.end(), '.', '$');
      std::vector<std::string> template_params;
      for (const auto &arg : c->args) {
        if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_TYPE) {
          kphp_assert(arg->flags & FLAG_OPT_VAR);
          template_params.emplace_back(format("typename T%d::PhpType", arg->var_num));
        }
      }
      std::string template_suf = template_params.empty() ? "" : "<" + vk::join(template_params, ", ") + ">";
      res = G->env().get_tl_classname_prefix() + (c->is_constructor() ? "Types$" : "Functions$") + name + template_suf;
  }
  if (wrap_to_class_instance) {
    return format("class_instance<%s>", res.c_str());
  }
  return res;
}

std::string get_php_runtime_type(const vk::tl::type *t) {
  if (t->name == "ReqResult") {
    return format("class_instance<%sRpcResponse>", G->env().get_tl_classname_prefix().c_str());
  }
  if (t->is_polymorphic()) {    // тогда пользуемся именем типа, а не конструктора
    return get_php_runtime_type(t->constructors[0].get(), true, t->name);
  }
  return get_php_runtime_type(t->constructors[0].get(), true);
}

std::string get_tl_object_field_access(const std::unique_ptr<vk::tl::arg> &arg) {
  kphp_assert(!arg->name.empty());
  return format("tl_object->$%s", arg->name.c_str());
}
} // namespace

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
void t_tree_stats_CountersTypesFilter::store(const var &tl_object) {
  const string &c_name = tl_arr_get(tl_object, tl_str$_, 0, -2147483553).to_string();
  if (c_name == tl_str$tree_stats$countersTypesFilterList) {
    f$store_int(0xe6749cee);
    c_tree_stats_countersTypesFilterList::store(tl_object);
  } else if (c_name == tl_str$tree_stats$countersTypesFilterRange) {
    f$store_int(0x9329d6e0);
    c_tree_stats_countersTypesFilterRange::store(tl_object);
  } else {
    CurrentProcessingQuery::get().raise_storing_error("Invalid constructor %s of type %s", c_name.c_str(), "tree_stats.CountersTypesFilter");
  }
}
*/
struct TypeStore {
  const std::unique_ptr<vk::tl::type> &type;
  std::string template_str;
  bool typed_mode;

  TypeStore(const std::unique_ptr<vk::tl::type> &type, string template_str, bool typed_mode = false) :
    type(type),
    template_str(std::move(template_str)),
    typed_mode(typed_mode) {}

  void compile(CodeGenerator &W) const {
    auto store_params = get_optional_args_for_call(type->constructors[0]);
    store_params.insert(store_params.begin(),
                        typed_mode ? (type->is_polymorphic() ? "conv_obj" : "tl_object.get()") : "tl_object");
    std::string store_call = (typed_mode ? "typed_store(" : "store(") + vk::join(store_params, ", ") + ");";

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
        ClassPtr php_class = get_php_class_of_tl_constructor(c.get());
        if (!php_class) {
          W << "// for " << c->name << " not found: " << get_php_runtime_type(c.get()) << NL;
          continue;
        }
        std::string cpp_class = get_php_runtime_type(c.get());

        W << (first ? "if " : " else if ");
        first = false;

        W << "(f$is_a<" << cpp_class << ">(tl_object)) " << BEGIN;
        if (c.get() != default_constructor) {
          W << format("f$store_int(0x%08x);", static_cast<unsigned int>(c->id)) << NL;
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
        << register_tl_const_str("_") << ", 0, " << int_to_str(hash_tl_const_str("_")) << ").to_string();" << NL;
      for (const auto &c : type->constructors) {
        W << (first ? "if " : " else if ");
        first = false;

        W << "(c_name == " << register_tl_const_str(c->name) << ") " << BEGIN;
        if (c.get() != default_constructor) {
          W << format("f$store_int(0x%08x);", static_cast<unsigned int>(c->id)) << NL;
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
array<var> t_tree_stats_CountersTypesFilter::fetch() {
  array<var> result;
  if (!CurException.is_null()) return result;
  unsigned int magic = static_cast<unsigned int>(f$fetch_int());
  switch(magic) {
    case 0xe6749cee: {
      result = c_tree_stats_countersTypesFilterList::fetch();
      result.set_value(tl_str$_, tl_str$tree_stats$countersTypesFilterList, -2147483553);
      break;
    }
    case 0x9329d6e0: {
      result = c_tree_stats_countersTypesFilterRange::fetch();
      result.set_value(tl_str$_, tl_str$tree_stats$countersTypesFilterRange, -2147483553);
      break;
    }
    default: {
      CurrentProcessingQuery::get().raise_fetching_error("Incorrect magic of type tree_stats.CountersTypesFilter: %08x", magic);
    }
  }
  return result;
}
 */
struct TypeFetch {
  const std::unique_ptr<vk::tl::type> &type;
  std::string template_str;
  bool typed_mode;

  inline TypeFetch(const std::unique_ptr<vk::tl::type> &type, string template_str, bool typed_mode = false) :
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
        W << "if (!CurException.is_null()) return;" << NL;
        W << get_php_runtime_type(constructor.get(), true) << " result;" << NL;
        W << "result.alloc();" << NL;
        // во все c_*.typed_fetch_to() приходит уже аллоцированный объект.
        // Он всегда аллоцируется в t_*.typed_fetch_to().
        W << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
        W << "tl_object = result;" << NL;
      } else {
        W << "if (!CurException.is_null()) return array<var>();" << NL;
        W << "return " << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
      }
      return;
    }

    // полиформные типы фетчатся сложнее: сначала magic, потом switch(magic) на каждый конструктор
    if (!typed_mode) {
      W << "array<var> result;" << NL;
    }
    W << "if (!CurException.is_null()) return" << (typed_mode ? "" : " result") << ";" << NL;
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
      if (typed_mode && !get_php_class_of_tl_constructor(c.get())) {
        continue;
      }
      
      W << format("case 0x%08x: ", static_cast<unsigned int>(c->id)) << BEGIN;
      if (!typed_mode) {
        W << "result = " << cpp_tl_struct_name("c_", c->name, template_str) << "::" << fetch_call << NL;
        if (has_name) {
          W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(c->name) << ", " << int_to_str(hash_tl_const_str("_")) << ");"
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
            << int_to_str(hash_tl_const_str("_")) << ");" << NL;
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
    return target_expr + format(".store(tl_arr_get(tl_object, %s, %d, %d))", register_tl_const_str(arg->name).c_str(), arg->idx, hash_tl_const_str(arg->name));
  } else {
    return target_expr + format(".typed_store(%s)", get_tl_object_field_access(arg).c_str());
  }
}

template<typename T>
std::string get_magic_storing(const T &arg_type_expr) {
  if (auto arg_as_type_expr = arg_type_expr->template as<vk::tl::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      return format("f$store_int(0x%08x);", static_cast<unsigned int>(type_of(arg_as_type_expr)->id));
    }
  } else if (auto arg_as_type_var = arg_type_expr->template as<vk::tl::type_var>()) {
    return format("store_magic_if_not_bare(inner_magic%d);", arg_as_type_var->var_num);
  }
  return "";
}

template<typename T>
std::string get_magic_fetching(const T &arg_type_expr, const char *error_msg) {
  // Здесь нельзя использовать format, так как в аргументы этой функции иногда уже приходит результат format
  char buf[1000];
  if (auto arg_as_type_expr = arg_type_expr->template as<vk::tl::type_expr>()) {
    if (is_magic_processing_needed(arg_as_type_expr)) {
      sprintf(buf, R"(fetch_magic_if_not_bare(0x%08x, "%s");)",
              static_cast<unsigned int>(type_of(arg_as_type_expr)->id), error_msg);
      return buf;
    }
  } else if (auto arg_as_type_var = arg_type_expr->template as<vk::tl::type_var>()) {
    for (const auto &arg : cur_combinator->args) {
      if (auto casted = arg->type_expr->as<vk::tl::type_var>()) {
        if ((arg->flags & FLAG_EXCL) && casted->var_num == arg_as_type_var->var_num) {
          return R"(fetch_magic_if_not_bare(0, "");)";
        }
      }
    }
    sprintf(buf, R"(fetch_magic_if_not_bare(inner_magic%d, "%s");)", arg_as_type_var->var_num, error_msg);
    return buf;
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
    std::string magic_storing = get_magic_storing(arg->type_expr);
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
    unique_object<tl_func_base> f_rpcProxy_diagonalTargets::store(const var& tl_object) {
      auto result_fetcher = make_unique_object<f_rpcProxy_diagonalTargets>();
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
  vk::tl::combinator *combinator;
  bool typed_mode;

  inline explicit CombinatorStore(vk::tl::combinator *combinator, bool typed_mode = false) :
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
      W << format("f$store_int(0x%08x);", static_cast<unsigned int>(combinator->id)) << NL;
    }
    for (const auto &arg : combinator->args) {
      // Если аргумент является параметром типа (обрамлен в {}), пропускаем его
      // Все такие аргументы хранятся в типе, как поля структуры
      if (arg->flags & FLAG_OPT_VAR) {
        continue;
      }
      // Если поле необязательное и зависит от филд маски
      if (arg->flags & FLAG_OPT_FIELD) {
        // 2 случая:
        //      1) Зависимость от значения, которым параметризуется тип (имеет смысл только для конструкторов)
        //      2) Зависимость от значения, которое получаем из явных аргументов
        W << format("if (%s%s & (1 << %d)) ",
                    var_num_access,
                    combinator->get_var_num_arg(arg->exist_var_num)->name.c_str(),
                    arg->exist_var_bit) << BEGIN;
        // полиморфно обрабатываются, так как запоминаем их все либо в локальные переменны либо в поля структуры
      }
      if (!(arg->flags & FLAG_EXCL)) {
        W << TypeExprStore(arg, var_num_access, typed_mode);
      }
      //Обработка восклицательного знака
      if (arg->flags & FLAG_EXCL) {
        kphp_assert(combinator->is_function());
        auto as_type_var = arg->type_expr->as<vk::tl::type_var>();
        kphp_assert(as_type_var);
        if (!typed_mode) {
          W << "auto _cur_arg = "
            << format("tl_arr_get(tl_object, %s, %d, %d)", register_tl_const_str(arg->name).c_str(), arg->idx, hash_tl_const_str(arg->name))
            << ";" << NL;
          W << "string target_f_name = " << format("tl_arr_get(_cur_arg, %s, 0, %d).as_string()", register_tl_const_str("_").c_str(), hash_tl_const_str("_"))
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
            << get_tl_object_field_access(arg) << ".get().store();" << NL;
        }
      } else if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_INTEGER_VARIABLE) {
        // Запоминаем филд маску для последующего использования
        // Может быть либо локальной переменной либо полем структуры
        if (!typed_mode) {
          W << format("%s%s = f$intval(tl_arr_get(tl_object, %s, %d, %d));",
                      var_num_access,
                      combinator->get_var_num_arg(arg->var_num)->name.c_str(),
                      register_tl_const_str(arg->name).c_str(),
                      arg->idx,
                      hash_tl_const_str(arg->name)) << NL;
        } else {
          W << var_num_access << combinator->get_var_num_arg(arg->var_num)->name << " = " << get_tl_object_field_access(arg) << ";" << NL;
        }
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        W << END << NL;
      }
    }
  }
};

std::string get_fetcher_call(const std::unique_ptr<vk::tl::type_expr_base> &type_expr, bool typed_mode = false, const std::string &storage = "") {
  if (!typed_mode) {
    return get_full_value(type_expr.get(), "") + ".fetch()";
  } else {
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
    const auto magic_fetching = get_magic_fetching(arg->type_expr,
                                                   format("Incorrect magic of arg: %s\\nin constructor: %s", arg->name.c_str(), cur_combinator->name.c_str()));
    if (!magic_fetching.empty()) {
      W << magic_fetching << NL;
    }
    if (!typed_mode) {
      W << "result.set_value(" << register_tl_const_str(arg->name) << ", " << get_fetcher_call(arg->type_expr) << ", "
        << int_to_str(hash_tl_const_str(arg->name))
        << ");" << NL;
    } else {
      W << get_fetcher_call(arg->type_expr, true, get_tl_object_field_access(arg)) << ";" << NL;
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
  vk::tl::combinator *combinator;
  bool typed_mode;

  explicit CombinatorFetch(vk::tl::combinator *combinator, bool typed_mode = false) :
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
        const auto &magic_fetching = get_magic_fetching(combinator->result,
                                                        format("Incorrect magic in result of function: %s", cur_combinator->name.c_str()));
        if (!magic_fetching.empty()) {
          W << magic_fetching << NL;
        }
      } else {
        W << format("fetch_magic_if_not_bare(0x%08x, \"%s\");",
                    combinator->original_result_constructor_id,
                    ("Incorrect magic in result of function: " + cur_combinator->name).c_str())
          << NL;
      }
      if (!typed_mode) {
        W << "return " << get_fetcher_call(combinator->result) << ";" << NL;
      } else {
        // для любой getChatInfo implements RpcFunction есть getChatInfo_result implements RpcFunctionReturnResult
        W << get_php_runtime_type(combinator, true, combinator->name + "_result") << " result;" << NL
          << "result.alloc();" << NL
          << get_fetcher_call(combinator->result, true, "result->$value") << ";" << NL
          << "return result;" << NL;
      }
      return;
    }
    if (!typed_mode) {
      W << "array<var> result;" << NL;
    }
    for (const auto &arg : combinator->args) {
      // Если аргумент является параметром типа (обрамлен в {}), пропускаем его
      // Все такие аргументы хранятся в типе, как поля структуры
      if (arg->flags & FLAG_OPT_VAR) {
        continue;
      }
      // Если поле необязательное и зависит от филд маски
      if (arg->flags & FLAG_OPT_FIELD) {
        W << format("if (%s & (1 << %d)) ",
                    combinator->get_var_num_arg(arg->exist_var_num)->name.c_str(),
                    arg->exist_var_bit) << BEGIN;
      }
      W << TypeExprFetch(arg, typed_mode);
      if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_INTEGER_VARIABLE) {
        // запоминаем филд маску для дальнейшего использования
        if (!typed_mode) {
          W << combinator->get_var_num_arg(arg->var_num)->name << " = f$intval(result.get_value(" << register_tl_const_str(arg->name) << ", "
            << int_to_str(hash_tl_const_str(arg->name)) << "));" << NL;
        } else {
          W << combinator->get_var_num_arg(arg->var_num)->name << " = " << get_tl_object_field_access(arg) << ";" << NL;
        }
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        W << END << NL;
      }
    }
    if (!typed_mode) {
      W << "return result;" << NL;
    }
  }
};

/* В тл есть свои массивы:
      search.restrictions m:# data:m*[rate_type:int min_value:int max_value:int] = search.Restrictions;
   Cell - это структура описывающая ячейку массива. (в данном примере [rate_type:int min_value:int max_value:int])
   Они очень редко используются в коде, поэтому в генерации есть некоторые предположения о том, что они достаточно простые.
Пример:
//search.restrictions
struct cell_14 {
  void store(const var& tl_object) {
    t_Int().store(tl_arr_get(tl_object, tl_str$rate_type, 1, -142719793));
    t_Int().store(tl_arr_get(tl_object, tl_str$min_value, 2, -1925326417));
    t_Int().store(tl_arr_get(tl_object, tl_str$max_value, 3, -1908137881));
  }

  array<var> fetch() {
    array<var> result;
    result.set_value(tl_str$rate_type, t_Int().fetch(), -142719793);
    result.set_value(tl_str$min_value, t_Int().fetch(), -1925326417);
    result.set_value(tl_str$max_value, t_Int().fetch(), -1908137881);
    return result;
  }

};
*/
struct Cell {
  static int cells_cnt;
  static std::unordered_map<vk::tl::type_array *, std::string> type_array_to_cell_name;

  vk::tl::combinator *owner;
  vk::tl::type_array *type_array;
  std::string name;

  Cell(vk::tl::combinator *owner, vk::tl::type_array *type_array) :
    owner(owner),
    type_array(type_array),
    name("cell_" + std::to_string(cells_cnt++)) {}

  void compile(CodeGenerator &W) const {
    cur_combinator = owner;
    const bool needs_typed_fetch_store = is_flat() ? false : !!get_cell_php_class();

    for (const auto &arg : type_array->args) {
      kphp_assert(!(arg->flags & (FLAG_OPT_VAR | FLAG_OPT_FIELD)) && arg->type_expr->as<vk::tl::type_expr>());
    }
    W << "//" + owner->name << NL;
    // Когда ячейка массива состоит из одного элемента, генерится псевдоним на этот тип вместо новой структуры
    if (is_flat()) {
      W << "using " << name << " = " << get_full_type(type_array->args[0]->type_expr.get(), "") << ";\n" << NL;
      return;
    }
    W << "struct " + name + " " << BEGIN;
    W << "using PhpType = " << (needs_typed_fetch_store ? get_cell_php_type() : "tl_undefined_php_type") << ";" << NL << NL;
    
    W << "void store(const var &tl_object) " << BEGIN;
    for (const auto &arg : type_array->args) {
      W << TypeExprStore(arg, "");
    }
    W << END << NL << NL;

    W << "array<var> fetch() " << BEGIN;
    W << "array<var> result;" << NL;
    for (const auto &arg : type_array->args) {
      W << TypeExprFetch(arg);
    }
    W << "return result;" << NL;
    W << END << NL << NL;

    if (needs_typed_fetch_store) {
      W << "void typed_store(const PhpType &tl_object) " << BEGIN;
      for (const auto &arg : type_array->args) {
        W << TypeExprStore(arg, "", true);
      }
      W << END << NL << NL;

      W << "void typed_fetch_to(PhpType &tl_object) " << BEGIN;
      for (const auto &arg : type_array->args) {
        W << TypeExprFetch(arg, true);
      }
      W << END << NL << NL;
    }

    W << END << ";\n" << NL;
  }

  // не-flat ячейке соответствует php-класс, например: VK\TL\Types\search\restrictions__arg2_item
  ClassPtr get_cell_php_class() const {
    kphp_assert(!is_flat());          // для флат cell'ов генерится using на inner

    int arg_idx = -1;
    for (const auto &arg: owner->args) {
      if (auto casted = dynamic_cast<vk::tl::type_array *>(arg->type_expr.get())) {
        if (type_array_to_cell_name[casted] == name) {
          arg_idx = arg->idx;
          break;
        }
      }
    }

    std::string php_class_name = get_php_runtime_type(owner) + format("__arg%d_item", arg_idx);
    return G->get_class(php_class_name);
  }

private:
  bool is_flat() const {
    return type_array->args.size() == 1;
  }

  std::string get_cell_php_type() const {
    ClassPtr php_class = get_cell_php_class();
    kphp_assert(php_class);
    return format("class_instance<%s>", php_class->src_name.c_str());
  }
};

int Cell::cells_cnt = 0;
std::unordered_map<vk::tl::type_array *, std::string> Cell::type_array_to_cell_name = std::unordered_map<vk::tl::type_array *, std::string>();

struct TlConstructorDecl {
  const std::unique_ptr<vk::tl::combinator> &constructor;

  static bool does_tl_constructor_need_typed_fetch_store(vk::tl::combinator *c) {
    return !!get_php_class_of_tl_constructor(c);
  }

  static std::string get_optional_args_for_decl(const std::unique_ptr<vk::tl::combinator> &c) {
    std::vector<std::string> res;
    for (const auto &arg : c->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        if (type_of(arg->type_expr)->name == T_INTEGER_VARIABLE) {
          res.emplace_back("int " + arg->name);
        } else {
          res.emplace_back("T" + std::to_string(arg->var_num) + " " + arg->name);
        }
      }
    }
    return vk::join(res, ", ");
  }

  explicit TlConstructorDecl(const std::unique_ptr<vk::tl::combinator> &constructor) :
    constructor(constructor) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor.get());

    std::string template_decl = get_template_declaration(constructor);
    if (!template_decl.empty()) {
      W << template_decl << NL;
    }
    W << "struct " << cpp_tl_struct_name("c_", constructor->name) << " " << BEGIN;
    auto params = get_optional_args_for_decl(constructor);
    W << "static void store(const var &tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
    W << "static array<var> fetch(" << params << ");" << NL;

    if (needs_typed_fetch_store) {
      std::string php_type = get_php_runtime_type(constructor.get(), false);
      W << "static void typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
      W << "static void typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ");" << NL;
    }
    W << END << ";\n\n";
  }
};

struct TlConstructorDef {
  const std::unique_ptr<vk::tl::combinator> &constructor;

  explicit TlConstructorDef(const std::unique_ptr<vk::tl::combinator> &constructor) :
    constructor(constructor) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor.get());

    std::string template_decl = get_template_declaration(constructor);
    std::string template_def = get_template_definition(constructor);
    auto full_struct_name = cpp_tl_struct_name("c_", constructor->name, template_def);
    auto params = TlConstructorDecl::get_optional_args_for_decl(constructor);

    W << template_decl << NL;
    W << "void " << full_struct_name + "::store(const var& tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
    W << CombinatorStore(constructor.get());
    W << END << "\n\n";

    W << template_decl << NL;
    W << "array<var> " << full_struct_name + "::fetch(" << params << ") " << BEGIN;
    W << CombinatorFetch(constructor.get());
    W << END << "\n\n";

    if (needs_typed_fetch_store) {
      std::string php_type = get_php_runtime_type(constructor.get(), false);

      W << template_decl << NL;
      W << "void " << full_struct_name + "::typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
      W << CombinatorStore(constructor.get(), true);
      W << END << "\n\n";

      W << template_decl << NL;
      W << "void " << full_struct_name << "::typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
      W << CombinatorFetch(constructor.get(), true);
      W << END << "\n\n";
    }
  }
};

struct TlTypeDecl {
  vk::tl::type *t;

  // tl-типу 'messages.ChatInfoUser' соответствует:
  // * либо php class VK\TL\Types\messages\chatInfoUser
  // * либо php interface VK\TL\Types\messages\ChatInfoUser, если тип полиморфный
  // todo понять, что с зависимыми типами; прокомментить про достижимость
  static bool does_tl_type_need_typed_fetch_store(vk::tl::type *t) {
    if (t->name == "ReqResult") {
      // без этого сайт не компилится, пока typed rpc не подключен — т.к. типизированный t_ReqResult не компилится
      bool typed_php_code_exists = !!G->get_class(G->env().get_tl_namespace_prefix() + "Types\\rpcResponseOk");
      return typed_php_code_exists;
    }
    std::string php_interface_name = G->env().get_tl_namespace_prefix() + "Types\\" + replace_characters(t->name, '.', '\\');
    if (G->get_class(php_interface_name)) {
      return true;
    }
    const size_t idx_last_slash = php_interface_name.rfind('\\');
    php_interface_name[idx_last_slash + 1] = tolower(php_interface_name[idx_last_slash + 1]);
    return !!G->get_class(php_interface_name);
  }

  explicit TlTypeDecl(vk::tl::type *t) :
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << t->name << " ~~~~~~~~~ */\n" << NL;
    const bool needs_typed_fetch_store = TlTypeDecl::does_tl_type_need_typed_fetch_store(t);

    std::string struct_name = cpp_tl_struct_name("t_", t->name);
    const auto &constructor = t->constructors[0];
    std::string template_decl = get_template_declaration(constructor);
    if (!template_decl.empty()) {
      W << template_decl << NL;
    }
    W << "struct " << struct_name << " " << BEGIN;
    W << "using PhpType = " << (needs_typed_fetch_store ? get_php_runtime_type(t) : "tl_undefined_php_type") << ";" << NL;
    
    std::vector<std::string> constructor_params;
    std::vector<std::string> constructor_inits;
    for (const auto &arg : constructor->args) {
      if (arg->var_num != -1) {
        if (type_of(arg->type_expr)->name == T_INTEGER_VARIABLE) {
          if (arg->flags & FLAG_OPT_VAR) {
            W << "int " << arg->name << "{0};" << NL;
            constructor_params.emplace_back("int " + arg->name);
            constructor_inits.emplace_back(format("%s(%s)", arg->name.c_str(), arg->name.c_str()));
          }
        } else if (type_of(arg->type_expr)->name == T_TYPE) {
          W << format("T%d %s;", arg->var_num, arg->name.c_str()) << NL;
          kphp_assert(arg->flags & FLAG_OPT_VAR);
          constructor_params.emplace_back(format("T%d %s", arg->var_num, arg->name.c_str()));
          constructor_inits.emplace_back(format("%s(std::move(%s))", arg->name.c_str(), arg->name.c_str()));
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
  vk::tl::type *t;

  explicit TlTypeDef(vk::tl::type *t) :
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlTypeDecl::does_tl_type_need_typed_fetch_store(t);

    const auto &constructor = t->constructors[0];
    std::string struct_name = cpp_tl_struct_name("t_", t->name);
    const auto &type = tl->types[constructor->type_id];
    std::string template_decl = get_template_declaration(constructor);
    std::string template_def = get_template_definition(constructor);
    auto full_struct_name = struct_name + template_def;

    W << template_decl << NL;
    W << "void " << full_struct_name << "::store(const var &tl_object) " << BEGIN;
    W << TypeStore(type, template_def);
    W << END << "\n\n";

    W << template_decl << NL;
    W << "array<var> " << full_struct_name + "::fetch() " << BEGIN;
    W << TypeFetch(type, template_def);
    W << END << "\n\n";

    if (needs_typed_fetch_store) {
      W << template_decl << NL;
      W << "void " << full_struct_name << "::typed_store(const PhpType &tl_object) " << BEGIN;
      W << TypeStore(type, template_def, true);
      W << END << "\n\n";

      W << template_decl << NL;
      W << "void " << full_struct_name + "::typed_fetch_to(PhpType &tl_object) " << BEGIN;
      W << TypeFetch(type, template_def, true);
      W << END << "\n\n";
    }
  }
};

struct TlFunctionDecl {
  vk::tl::combinator *f;

  static bool does_tl_function_need_typed_fetch_store(vk::tl::combinator *f) {
    return !!get_php_class_of_tl_function(f);
  }

  explicit TlFunctionDecl(vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << f->name << " ~~~~~~~~~ */\n" << NL;
    const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);

    std::string struct_name = cpp_tl_struct_name("f_", f->name);
    W << "struct " + struct_name + " : tl_func_base " << BEGIN;
    for (const auto &arg : f->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        W << "tl_exclamation_fetch_wrapper " << arg->name << ";" << NL;
      } else if (arg->var_num != -1) {
        W << "int " << arg->name << "{0};" << NL;
      }
    }
    W << "static unique_object<tl_func_base> store(const var &tl_object);" << NL;
    W << "var fetch();" << NL;
    if (needs_typed_fetch_store) {
      W << "static unique_object<tl_func_base> typed_store(const " << get_php_runtime_type(f) << " *tl_object);" << NL;
      W << "class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> typed_fetch();" << NL;
    }
    W << END << ";" << NL << NL;
  }
};

class TlFunctionDef {
  vk::tl::combinator *f;

public:
  explicit TlFunctionDef(vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);
    std::string struct_name = cpp_tl_struct_name("f_", f->name);

    W << "unique_object<tl_func_base> " << struct_name << "::store(const var& tl_object) " << BEGIN;
    W << "auto result_fetcher = make_unique_object<" << struct_name << ">();" << NL;
    W << CombinatorStore(f);
    W << "return std::move(result_fetcher);" << NL;
    W << END << NL << NL;

    W << "var " << struct_name << "::fetch() " << BEGIN;
    W << CombinatorFetch(f);
    W << END << NL << NL;

    if (needs_typed_fetch_store) {
      W << "unique_object<tl_func_base> " << struct_name << "::typed_store(const " << get_php_runtime_type(f) << " *tl_object) " << BEGIN;
      W << "auto result_fetcher = make_unique_object<" << struct_name << ">();" << NL;
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
  std::vector<Cell> cells;
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
        W << TlConstructorDecl(constructor);
      }
    }
    for (const auto &t : target_types) {
      W << TlTypeDecl(t);
      if (is_type_dependent(t)) {
        W << TlTypeDef(t);
      }
    }
    for (const auto &cell : cells) {
      W << cell;
    }
    for (const auto &t : target_types) {
      if (is_type_dependent(t)) {
        for (const auto &constructor : t->constructors) {
          W << TlConstructorDef(constructor);
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
      if (!is_type_dependent(t)) {
        W << TlTypeDef(t);
        for (const auto &constructor : t->constructors) {
          W << TlConstructorDef(constructor);
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
      h_includes.add_class_forward_declaration(get_php_class_of_tl_function(f.get()));
      cpp_includes.add_class_include(get_php_class_of_tl_function(f.get()));
      cpp_includes.add_class_include(get_php_class_of_tl_function_result(f.get()));
    }
  }

  void add_obj(const std::unique_ptr<vk::tl::type> &t) {
    target_types.push_back(t.get());
    update_dependencies(t);

    for (const auto &c : t->constructors) {
      if (TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(c.get())) {
        h_includes.add_class_include(get_php_class_of_tl_constructor(c.get()));
      }
    }
  }

  void add_obj(const Cell &cell) {
    cells.push_back(cell);
    update_dependencies(cell);
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

  static void add_to_module(const Cell &cell) {
    ensure_existence(get_module_name(cell.owner->name)).add_obj(cell);
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

  void update_dependencies(const Cell &cell) {
    for (const auto &arg : cell.type_array->args) {
      collect_deps_from_type_tree(arg->type_expr.get());
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
std::pair<std::string, std::string> get_full_type_expr_str(
  vk::tl::expr_base *type_expr, const std::string &var_num_access) {
  auto as_nat_var = type_expr->as<vk::tl::nat_var>();
  if (as_nat_var) {
    return {"", var_num_access + cur_combinator->get_var_num_arg(as_nat_var->var_num)->name};
  }
  auto as_nat_const = type_expr->as<vk::tl::nat_const>();
  if (type_expr->as<vk::tl::nat_const>()) {
    return {"", std::to_string(as_nat_const->num)};
  }
  auto as_type_var = type_expr->as<vk::tl::type_var>();
  if (as_type_var) {
    std::string expr = "std::move(" + cur_combinator->get_var_num_arg(as_type_var->var_num)->name + ")";
    if (cur_combinator->is_constructor()) {
      return {"T" + std::to_string(as_type_var->var_num), expr};
    } else {
      return {"tl_exclamation_fetch_wrapper", expr};
    }
  }
  auto as_type_array = type_expr->as<vk::tl::type_array>();
  if (as_type_array) {
    std::string type = format("tl_array<%s>", Cell::type_array_to_cell_name[as_type_array].c_str());
    return {type, type + format("(%s, %s())",
                                get_full_value(as_type_array->multiplicity.get(), var_num_access).c_str(),
                                Cell::type_array_to_cell_name[as_type_array].c_str())};
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
          template_params.emplace_back(format("0x%08x", static_cast<unsigned int>(type_of(child_as_type_expr)->id)));
        } else {
          template_params.emplace_back("0");
        }
      } else if (auto child_as_type_var = child->as<vk::tl::type_var>()) {
        if (vk::string_view(child_type_name).starts_with("T")) {
          template_params.emplace_back(format("inner_magic%d", child_as_type_var->var_num));
        } else {
          template_params.emplace_back("0");
        }
      } else {
        kphp_assert(child->as<vk::tl::type_array>());
        template_params.emplace_back("0");
      }
    }
  }
  auto type_name = (type->name != T_INTEGER_VARIABLE ? cpp_tl_struct_name("t_", type->name) : cpp_tl_struct_name("t_", tl->types[TL_INT]->name));
  auto template_str = (!template_params.empty() ? "<" + vk::join(template_params, ", ") + ">" : "");
  auto full_type_name = type_name + template_str;
  auto full_type_value = full_type_name + "(" + vk::join(constructor_params, ", ") + ")";
  return {full_type_name, full_type_value};
}

void check_constructor(const std::unique_ptr<vk::tl::combinator> &c) {
  // Проверяем, что порядок неявных аргументов конструктора совпадает с их порядком в типе
  std::vector<int> var_nums;
  for (const auto &arg : c->args) {
    if ((arg->flags & FLAG_OPT_VAR) && arg->var_num != -1) {
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
             format("Strange tl scheme here: %s", c->name.c_str()));
}

void collect_cells(vk::tl::expr_base *expr) {
  if (auto as_type_expr = expr->as<vk::tl::type_expr>()) {
    for (const auto &child : as_type_expr->children) {
      collect_cells(child.get());
    }
  } else if (auto as_type_array = expr->as<vk::tl::type_array>()) {
    auto cell = Cell(cur_combinator, as_type_array);
    Cell::type_array_to_cell_name[as_type_array] = cell.name;
    Module::add_to_module(cell);
    for (const auto &arg : as_type_array->args) {
      collect_cells(arg->type_expr.get());
    }
  }
}

/* Разбиваем все комбинаторы и типы на модули, вместе с тем собирая зависимости каждого модуля.
 * Здесь же собираем все cell'ы (см. описание у структуры Cell).
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
        for (const auto &arg : c->args) {
          collect_cells(arg->type_expr.get());
        }
      }
    }
  }

  for (const auto &e : tl->functions) {
    const std::unique_ptr<vk::tl::combinator> &f = e.second;
    Module::add_to_module(f);
    cur_combinator = f.get();
    for (const auto &arg : f->args) {
      collect_cells(arg->type_expr.get());
    }
  }
}

void write_tl_query_handlers(CodeGenerator &W) {
  if (G->env().get_tl_schema_file().empty()) {
    return;
  }

  auto tl_ptr = vk::tl::parse_tlo(G->env().get_tl_schema_file().c_str());
  kphp_error_return(tl_ptr.has_value(),
                    format("Error while reading tlo: %s", tl_ptr.error().c_str()));

  tl = tl_ptr.value().get();
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
  W << "array<var> gen$tl_fetch_wrapper(unique_object<tl_func_base> stored_fetcher) " << BEGIN
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
        << int_to_str(hash_tl_const_str(f->name)) << ");" << NL;
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
    W << cpp_tl_const_str(s) << ".assign_raw (&raw[" << std::to_string(const_string_shifts[i++]) << "]);" << NL;
  });
  W << END;
  W << CloseFile();
}
} // namespace tl_gen

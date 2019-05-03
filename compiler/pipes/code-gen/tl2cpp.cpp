#include "compiler/pipes/code-gen/tl2cpp.h"

#include <string>

#include "common/algorithms/string-algorithms.h"
#include "common/tlo-parsing/tlo-parsing-tools.h"

#include "compiler/code-gen/includes.h"
#include "compiler/compiler-core.h"
#include "compiler/pipes/code-gen/common-code-gen.h"

namespace tl_gen {
static const std::string T_TYPE = "Type";
static const std::string T_SHARP = "#";

using vk::tl::FLAG_OPT_FIELD;
using vk::tl::FLAG_OPT_VAR;
using vk::tl::FLAG_DEFAULT_CONSTRUCTOR;
using vk::tl::FLAG_NOCONS;
using vk::tl::FLAG_EXCL;
using vk::tl::FLAG_BARE;

static vk::tl::tl_scheme *tl;
static vk::tl::combinator *cur_combinator;
const bool DEBUG_MODE = false;

const std::unordered_set<std::string> CUSTOM_IMPL_TYPES
  {T_SHARP, T_TYPE, "Int", "Long", "Double", "String", "Bool",
   "False", "Vector", "Maybe",
   "Dictionary", "DictionaryField", "IntKeyDictionary", "IntKeyDictionaryField", "LongKeyDictionary", "LongKeyDictionaryField", "Tuple"};

namespace {
vk::tl::type *type_of(const std::unique_ptr<vk::tl::type_expr_base> &type_expr) {
  if (auto casted = type_expr->as<vk::tl::type_expr>()) {
    return tl->types[casted->type_id].get();
  }
  return nullptr;
}

vk::tl::arg *get_first_explicit_arg(vk::tl::combinator *combinator) {
  for (const auto &arg : combinator->args) {
    if (!(arg->flags & FLAG_OPT_VAR)) {
      return arg.get();
    }
  }
  return nullptr;
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

static std::string cpp_tl_struct_name(std::string tl_name) {
  std::replace(tl_name.begin(), tl_name.end(), '.', '_');
  return tl_name;
}

/* При генерации выделены 3 основные сущности, у каждой из которых есть методы store и fetch:
 * 1) Функция     - entry point любого tl запроса. Из него начинается де\сериализация любого запроса.
 *                  Имеет поля/аргументы (имена таких сгенеренных структур начинаются с f_).
 * 2) Тип         - свойтсво поля любого комбинатора (Функции или Конструктора).
 *                  де\сериализация любого поля начинается с вызова store/fetch у соответствующего Типа (struct t_...).
 *                  Реализация представляет собой switch по magic'у и перевызов соответствующего метода Конструктора.
 * 3) Конструктор - один из вариантов наборов данных для конкретного типа ("слагаемое" из суммы типов).
 *                  store/fetch у Конструктора вызывается ТОЛЬКО из store/fetch соответствующего типа.
 */

// Пример сгенеренного кода:
/*
void t_tree_stats_CountersTypesFilter::store(const var& tl_object) {
  const string &constructor_name = f$strval(tl_arr_get(tl_object, tl_str$_, 0));
  if (constructor_name == tl_str$tree_stats_countersTypesFilterList) {
    f$store_int(0xe6749cee);
    c_tree_stats_countersTypesFilterList().store(tl_object);
  } else if (constructor_name == tl_str$tree_stats_countersTypesFilterRange) {
    f$store_int(0x9329d6e0);
    c_tree_stats_countersTypesFilterRange().store(tl_object);
  } else {
    tl_storing_error(tl_object, "Constructor %s of type tree_stats.CountersTypesFilter was not found in TL scheme", constructor_name.c_str());
    return;
  }
}
*/
struct TypeStore {
  bool is_bare_type;
  const std::unique_ptr<vk::tl::type> &type;
  std::vector<std::string> constructor_params;
  std::string template_str;

  inline TypeStore(bool is_bare_type, const std::unique_ptr<vk::tl::type> &type, vector<string> constructor_params, string template_str) :
    is_bare_type(is_bare_type),
    type(type),
    constructor_params(std::move(constructor_params)),
    template_str(std::move(template_str)) {}

  inline void compile(CodeGenerator &W) const {
    if (type->constructors_num == 1) {
      if (is_bare_type) {
        W << "c_" << cpp_tl_struct_name(type->constructors[0]->name) << template_str << "(" << join(constructor_params, ", ") << ").store(tl_object);" << NL;
      } else {
        W << format("f$store_int(0x%08x);", static_cast<unsigned int>(type->constructors[0]->id)) << NL;
        W << "c_" << cpp_tl_struct_name(type->constructors[0]->name) << template_str << "(" << join(constructor_params, ", ") << ").store(tl_object);" << NL;
      }
      return;
    }
    W << "const string &constructor_name = f$strval(tl_arr_get(tl_object, " << register_tl_const_str("_") << ", 0, " << int_to_str(hash_tl_const_str("_")) << "));" << NL;
    auto default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
    for (const auto &c : type->constructors) {
      if (c == type->constructors[0]) {
        W << "if ";
      } else {
        W << END << " else if ";
      }
      W << "(constructor_name == " << register_tl_const_str(c->name) << ") " << BEGIN;
      if (c.get() != default_constructor) {
        W << format("f$store_int(0x%08x);", static_cast<unsigned int>(c->id)) << NL;
      }
      W << "c_" << cpp_tl_struct_name(c->name) << template_str << "(" << join(constructor_params, ", ") << ").store(tl_object);" << NL;
    }
    W << END << " else " << BEGIN
      << "tl_storing_error(tl_object, \"Constructor %s of type " << type->name << " was not found in TL scheme\", constructor_name.c_str());" << NL
      << "return;" << NL
      << END << NL;
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
      result = c_tree_stats_countersTypesFilterList().fetch();
      result.set_value(tl_str$_, tl_str$tree_stats_countersTypesFilterList);
      break;
    }
    case 0x9329d6e0: {
      result = c_tree_stats_countersTypesFilterRange().fetch();
      result.set_value(tl_str$_, tl_str$tree_stats_countersTypesFilterRange);
      break;
    }
    default: {
      tl_fetching_error("Incorrect magic of type tree_stats.CountersTypesFilter", "Incorrect magic of type tree_stats.CountersTypesFilter: %08x", magic);
    }
  }
  return result;
}
 */
struct TypeFetch {
  bool is_bare_type;
  const std::unique_ptr <vk::tl::type> &type;
  std::vector<std::string> constructor_params;
  std::string template_str;

  inline TypeFetch(bool is_bare_type, const std::unique_ptr <vk::tl::type> &type, vector<string> constructor_params, string template_str) :
    is_bare_type(is_bare_type),
    type(type),
    constructor_params(std::move(constructor_params)),
    template_str(std::move(template_str)) {}

  inline void compile(CodeGenerator &W) const {
    if (is_bare_type) {
      kphp_assert(type->constructors_num == 1);
      W << "if (!CurException.is_null()) return array<var>();" << NL;
      W << "return c_" << cpp_tl_struct_name(type->constructors[0]->name) << template_str << "(" << join(constructor_params, ", ") << ").fetch();" << NL;
      return;
    }
    W << (type->constructors[0]->is_flat(tl) ? "var " : "array<var> ");
    W << "result;" << NL;
    W << "if (!CurException.is_null()) return result;" << NL;
    auto default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
    bool has_name = type->constructors_num > 1 && !(type->flags & FLAG_NOCONS);
    if (default_constructor != nullptr) {
      W << "int pos = tl_parse_save_pos();" << NL;
    }
    W << "unsigned int magic = static_cast<unsigned int>(f$fetch_int());" << NL;
    W << "switch(magic) " << BEGIN;
    for (const auto &c : type->constructors) {
      if (c.get() == default_constructor) {
        continue;
      }
      W << format("case 0x%08x: ", static_cast<unsigned int>(c->id)) << BEGIN;
      W << "result = c_" << cpp_tl_struct_name(c->name) << template_str << "(" << join(constructor_params, ", ") << ").fetch();" << NL;
      if (has_name) {
        W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(c->name) << ", " << int_to_str(hash_tl_const_str("_")) << ");" << NL;
      }
      W << "break;" << NL;
      W << END << NL;
    }
    W << "default: " << BEGIN;
    if (default_constructor != nullptr) {
      W << "tl_parse_restore_pos(pos);" << NL;
      W << "result = c_" << cpp_tl_struct_name(default_constructor->name) << template_str << "(" << join(constructor_params, ", ") << ").fetch();" << NL;
      if (has_name) {
        W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(default_constructor->name) << ", " << int_to_str(hash_tl_const_str("_")) << ");" << NL;
      }
    } else {
      W << "tl_fetching_error(\"Incorrect magic of type " << type->name << "\", \"Incorrect magic of type " << type->name << ": %08x\", magic);" << NL;
    }
    W << END << NL;
    W << END << NL;
    W << "return result;" << NL;
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

std::string get_storer_call(const std::unique_ptr<vk::tl::arg> &arg, const std::string &var_num_access) {
  std::string res = get_full_value(arg->type_expr.get(), var_num_access);
  return res + format(".store(tl_arr_get(tl_object, %s, %d, %d))", register_tl_const_str(arg->name).c_str(), arg->idx, hash_tl_const_str(arg->name));
}

// Структура для генерации store любого type expression'а
struct TypeExprStore {
  const std::unique_ptr<vk::tl::arg> &arg;
  std::string var_num_access;

  inline TypeExprStore(const std::unique_ptr<vk::tl::arg> &arg, std::string var_num_access) :
    arg(arg),
    var_num_access(std::move(var_num_access)) {}

  inline void compile(CodeGenerator &W) const {
    W << get_storer_call(arg, var_num_access) << ";" << NL;
  }
};

/* Общий код для генерации store(...) для комбинаторов (то есть функций или контсрукторов).
 * Содержит основную логику.
 * Есть несколько частей:
 * 1) Обработка филд масок
    void c_hints_objectExt::store(const var& tl_object) {
      (void)tl_object;
      t_Int_$().store(tl_arr_get(tl_object, tl_str$type, 2));
      t_Int_$().store(tl_arr_get(tl_object, tl_str$object_id, 3));
      if (fields_mask & (1 << 0)) {
        t_Double_$().store(tl_arr_get(tl_object, tl_str$rating, 4));
      }
      if (fields_mask & (1 << 1)) {
        t_String_$().store(tl_arr_get(tl_object, tl_str$text, 5));
      }
    }
 * 2) Обработка восклицательных знаков:
    std::unique_ptr<tl_func_base> f_rpcProxy_diagonalTargets::store(const var& tl_object) {
      std::unique_ptr<f_rpcProxy_diagonalTargets> result_fetcher(new f_rpcProxy_diagonalTargets());
      (void)tl_object;
      f$store_int(0xee090e42);
      t_Int_$().store(tl_arr_get(tl_object, tl_str$offset, 2));
      t_Int_$().store(tl_arr_get(tl_object, tl_str$limit, 3));
      string target_f_name = tl_arr_get(tl_arr_get(tl_object, tl_str$query, 4), tl_str$_, 0).as_string();
      if (!tl_storers_ht.has_key(target_f_name)) {
        tl_storing_error(tl_object, "Function %s not found in tl-scheme", target_f_name.c_str());
        return nullptr;
      }
      const auto &storer_kv = tl_storers_ht.get_value(target_f_name);
      result_fetcher->X.fetcher = storer_kv(tl_arr_get(tl_object, tl_str$query, 4));
      return std::move(result_fetcher);
    }
 * 3) Обработка основной части типового выражения в TypeExprStore / Fetch
*/
struct CombinatorStore {
  vk::tl::combinator *combinator;

  inline explicit CombinatorStore(vk::tl::combinator *combinator) :
    combinator(combinator) {}

  inline void compile(CodeGenerator &W) const {
    cur_combinator = combinator;
    auto var_num_access = (combinator->is_function() ? "result_fetcher->" : "");
    if (combinator->is_flat(tl)) {
      W << get_full_value(get_first_explicit_arg(combinator)->type_expr.get(), var_num_access) << ".store(tl_object);" << NL;
      return;
    }
    W << "(void)tl_object;" << NL;
    if (combinator->is_function()) {
      W << format("f$store_int(0x%08x);", static_cast<unsigned int>(combinator->id)) << NL;
    }
    for (const auto &arg : combinator->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        continue;
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        // 2 случая:
        //      1) Зависимость от значения, которым параметризуется тип (имеет смысл только для конструкторов)
        //      2) Зависимость от значения, которое получаем в из явных аргументов
        W << format("if (%s%s & (1 << %d)) ",
                    var_num_access,
                    combinator->get_var_num_arg(arg->exist_var_num)->name.c_str(),
                    arg->exist_var_bit) << BEGIN;
        // полиморфно обрабатываются, так как запоминаем все var_num'ы
      }
      if (!(arg->flags & FLAG_EXCL)) {
        W << TypeExprStore(arg, var_num_access);
      }
      // запоминаем var_num для последующего использования
      if (arg->flags & FLAG_EXCL) {
        auto as_type_var = arg->type_expr->as<vk::tl::type_var>();
        kphp_assert(as_type_var);
        std::string cur_arg = format("tl_arr_get(tl_object, %s, %d, %d)", register_tl_const_str(arg->name).c_str(), arg->idx, hash_tl_const_str(arg->name));
        std::string cur_f_name = format("tl_arr_get(%s, %s, 0, %d).as_string()", cur_arg.c_str(), register_tl_const_str("_").c_str(), hash_tl_const_str("_"));
        if (DEBUG_MODE) {
          W << R"(fprintf(stderr, "!!! exclamation processing...\n");)" << NL;
        }
        W << "string target_f_name = " << cur_f_name << ";" << NL;
        W << "if (!tl_storers_ht.has_key(target_f_name)) " << BEGIN
          << "tl_storing_error(tl_object, \"Function %s not found in tl-scheme\", target_f_name.c_str());" << NL
          << "return nullptr;" << NL
          << END << NL
          << "const auto &storer_kv = tl_storers_ht.get_value(target_f_name);" << NL;
        W << var_num_access << combinator->get_var_num_arg(as_type_var->var_num)->name <<
          ".fetcher = storer_kv(" << cur_arg << ");" << NL;
      } else if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_SHARP) {
        W << format("%s%s = f$intval(tl_arr_get(tl_object, %s, %d, %d));",
                    var_num_access,
                    combinator->get_var_num_arg(arg->var_num)->name.c_str(),
                    register_tl_const_str(arg->name).c_str(),
                    arg->idx,
                    hash_tl_const_str(arg->name)) << NL;
        if (DEBUG_MODE) {
          W << R"(fprintf(stderr, "### fields_mask processing\n");)" << NL;
        }
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        W << END << NL;
      }
    }
  }
};

std::string get_fetcher_call(const std::unique_ptr<vk::tl::type_expr_base> &type_expr) {
  return get_full_value(type_expr.get(), "") + ".fetch()";
}

// Структура для генерации fetch любого type expression'а
struct TypeExprFetch {
  const std::unique_ptr<vk::tl::arg> &arg;

  explicit inline TypeExprFetch(const std::unique_ptr<vk::tl::arg> &arg) :
    arg(arg) {}

  inline void compile(CodeGenerator &W) const {
    W << "result.set_value(" << register_tl_const_str(arg->name) << ", " << get_fetcher_call(arg->type_expr) << ", " << int_to_str(hash_tl_const_str(arg->name)) << ");" << NL;
  }
};

/* Общий код для генерации fetch(...) для комбинаторов (то есть функций или контсрукторов).
 * Содержит основную логику.
 * Есть несколько частей:
 * 1) Обработка филд масок:
  array<var> c_hints_objectExt::fetch() {
    array<var> result;
    result.set_value(tl_str$type, t_Int_$().fetch());
    result.set_value(tl_str$object_id, t_Int_$().fetch());
    if (fields_mask & (1 << 0)) {
      result.set_value(tl_str$rating, t_Double_$().fetch());
    }
    if (fields_mask & (1 << 1)) {
      result.set_value(tl_str$text, t_String_$().fetch());
    }
    return result;
  }
 * 2) Обработка восклицательных знаков:
    var f_rpcProxy_diagonalTargets::fetch() {
      return t_Vector<t_Vector_$<t_Maybe<tl_exclamation_fetch_wrapper>>>(t_Vector_$<t_Maybe<tl_exclamation_fetch_wrapper>>(t_Maybe<tl_exclamation_fetch_wrapper>(X))).fetch();
    }
 * 3) Обработка основной части типового выражения в TypeExprStore / Fetch
*/
struct CombinatorFetch {
  vk::tl::combinator *combinator;

  inline explicit CombinatorFetch(vk::tl::combinator *combinator) :
    combinator(combinator) {}

  inline void compile(CodeGenerator &W) const {
    cur_combinator = combinator;
    if (combinator->is_flat(tl)) {
      W << "return " << get_fetcher_call(get_first_explicit_arg(combinator)->type_expr) << ";" << NL;
      return;
    }
    if (combinator->is_function()) {
      W << "return " << get_fetcher_call(combinator->result) << ";" << NL;
      return;
    }
    W << "array<var> result;" << NL;
    for (const auto &arg : combinator->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        continue;
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        W << format("if (%s & (1 << %d)) ",
                    combinator->get_var_num_arg(arg->exist_var_num)->name.c_str(),
                    arg->exist_var_bit) << BEGIN;
      }
      W << TypeExprFetch(arg);
      // запоминаем var_num для fields_mask
      if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_SHARP) {
        W << combinator->get_var_num_arg(arg->var_num)->name << " = f$intval(result.get_value(" << register_tl_const_str(arg->name) << ", " << int_to_str(hash_tl_const_str(arg->name)) << "));" << NL;
        if (DEBUG_MODE) {
          W << R"(fprintf(stderr, "### fields_mask processing\n");)" << NL;
        }
      }
      if (arg->flags & FLAG_OPT_FIELD) {
        W << END << NL;
      }
    }
    W << "return result;" << NL;
  }
};

/* В тл есть свои массивы:
      search.restrictions m:# data:m*[rate_type:int min_value:int max_value:int] = search.Restrictions;
   Cell - это структура описывающая ячейку массива. (в данном примере [rate_type:int min_value:int max_value:int])
   Они очень редко используются в коде, поэтому в генерации есть некоторые предположения о том, что они достаточно простые.
Пример:
struct cell_6 {
  void store(const var& tl_object) {
    t_Int_$().store(tl_arr_get(tl_object, tl_str$rate_type, 1));
    t_Int_$().store(tl_arr_get(tl_object, tl_str$min_value, 2));
    t_Int_$().store(tl_arr_get(tl_object, tl_str$max_value, 3));
  }

  array<var> fetch() {
    array<var> result;
    result.set_value(tl_str$rate_type, t_Int_$().fetch());
    result.set_value(tl_str$min_value, t_Int_$().fetch());
    result.set_value(tl_str$max_value, t_Int_$().fetch());
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

  inline Cell(vk::tl::combinator *owner, vk::tl::type_array *type_array) :
    owner(owner),
    type_array(type_array) {
    name = "cell_" + std::to_string(cells_cnt++);
  }

  inline void compile(CodeGenerator &W) const {
    cur_combinator = owner;
    for (const auto &arg : type_array->args) {
      kphp_assert(!(arg->flags & (FLAG_OPT_VAR | FLAG_OPT_FIELD)) && arg->type_expr->as<vk::tl::type_expr>());
    }
    W << "//" + owner->name << NL;
    // Когда ячейка массива состоит из одного элемента, генерится псевдоним на этот тип, вместо новой структуры
    if (type_array->args.size() == 1) {
      W << "using " << name << " = " << get_full_type(type_array->args[0]->type_expr.get(), "") << ";\n" << NL;
      return;
    }
    W << "struct " + name + " " << BEGIN;
    W << "void store(const var& tl_object) " << BEGIN;
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
    W << END << ";\n" << NL;
  }
};

int Cell::cells_cnt = 0;
std::unordered_map<vk::tl::type_array *, std::string> Cell::type_array_to_cell_name = std::unordered_map<vk::tl::type_array *, std::string>();

struct WrapperConstructorAndFields {
  const std::unique_ptr<vk::tl::combinator> &constructor;
  std::string name;

  inline WrapperConstructorAndFields(const std::unique_ptr<vk::tl::combinator> &constructor, string name) :
    constructor(constructor),
    name(std::move(name)) {};

  inline void compile(CodeGenerator &W) const {
    std::vector<std::string> constructor_params;
    std::vector<std::string> constructor_inits;
    std::vector<std::string> constructor_params_forward;
    for (const auto &arg : constructor->args) {
      if (arg->var_num != -1) {  // для всех TLTypeVar и TLTypeArray arg.var_num == -1
        if (type_of(arg->type_expr)->name == "#") {
          W << "int " << arg->name << "{0};" << NL;
          if (arg->flags & FLAG_OPT_VAR) {
            constructor_params.emplace_back("int " + arg->name);
            constructor_params_forward.emplace_back(arg->name);
            constructor_inits.emplace_back(format("%s(%s)", arg->name.c_str(), arg->name.c_str()));
          }
        } else if (type_of(arg->type_expr)->name == T_TYPE) {
          W << format("T%d %s;", arg->var_num, arg->name.c_str()) << NL;
          kphp_assert(arg->flags & FLAG_OPT_VAR);
          constructor_params.emplace_back(format("T%d %s", arg->var_num, arg->name.c_str()));
          constructor_params_forward.emplace_back(arg->name);
          constructor_inits.emplace_back(format("%s(std::move(%s))", arg->name.c_str(), arg->name.c_str()));
        }
      }
    }
    if (constructor_params.empty()) {
      return;
    }
    W << "explicit " << name << "(" << join(constructor_params, ", ") << ") : " << join(constructor_inits, ", ") << " {}\n" << NL;
  }
};

// Общий код для генерации объявления типов и контсрукторов
struct WrapperDecl {
  const std::unique_ptr<vk::tl::combinator> &constructor;
  std::string struct_name;

  inline WrapperDecl(const std::unique_ptr<vk::tl::combinator> &constructor, std::string struct_name) :
    constructor(constructor),
    struct_name(std::move(struct_name)) {}

  inline void compile(CodeGenerator &W) const {
    std::vector<std::string> typenames;
    if (is_type_dependent(constructor.get())) {
      for (const auto &arg : constructor->args) {
        if (arg->var_num != -1 && type_of(arg->type_expr)->name == T_TYPE) {
          kphp_assert(arg->flags & FLAG_OPT_VAR);
          typenames.emplace_back(format("typename T%d", arg->var_num));
        }
      }
      W << "template<" << join(typenames, ", ") << ">" << NL;
    }
    W << "struct " << struct_name << " " << BEGIN
      << WrapperConstructorAndFields(constructor, struct_name)
      << "void store(const var& tl_object);" << NL;
    if (constructor->is_flat(tl)) {
      W << "var fetch();" << NL;
    } else {
      W << "array<var> fetch();" << NL;
    }
    W << END << ";\n\n";
  }
};

// Общий код для генерации определения типов и контсрукторов
struct WrapperDef {
  const std::unique_ptr<vk::tl::combinator> &constructor;
  std::string struct_name;

  inline WrapperDef(const std::unique_ptr<vk::tl::combinator> &constructor, std::string struct_name) :
    constructor(constructor),
    struct_name(std::move(struct_name)) {}

  inline void compile(CodeGenerator &W) const {
    bool is_constructor = vk::string_view(struct_name).starts_with("c_");
    bool is_type = vk::string_view(struct_name).starts_with("t_");
    bool is_bare_type = is_type && vk::string_view(struct_name).ends_with("_$");
    const auto &type = tl->types[constructor->type_id];

    std::vector<std::string> constructor_params_forward;
    std::vector<std::string> template_params_forward;
    for (const auto &arg : constructor->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        if (type_of(arg->type_expr)->name == T_TYPE) {
          constructor_params_forward.emplace_back("std::move(" + arg->name + ")");
          template_params_forward.emplace_back("T" + std::to_string(arg->var_num));
        } else {
          constructor_params_forward.emplace_back(arg->name);
        }
      }
    }
    std::vector<std::string> typenames;
    typenames.reserve(template_params_forward.size());
    for (const auto &e : template_params_forward) {
      typenames.emplace_back("typename " + e);
    }
    std::string template_decl;
    std::string template_brackets;
    if (!typenames.empty()) {
      template_decl = format("template<%s>", join(typenames, ", ").c_str());
      template_brackets = format("<%s>", join(template_params_forward, ", ").c_str());
    }

    auto full_struct_name = struct_name + template_brackets;
    W << template_decl << NL;
    W << "void " + full_struct_name + "::store(const var& tl_object) " << BEGIN;
    if (is_constructor) {
      W << CombinatorStore(constructor.get());
    } else if (is_type) {
      W << TypeStore(is_bare_type, type, constructor_params_forward, template_brackets);
    } else {
      kphp_assert(false);
    }
    W << END << "\n\n";
    W << template_decl << NL;
    W << (constructor->is_flat(tl) ? "var " : "array<var> ");
    W << full_struct_name + "::fetch() " << BEGIN;
    if (is_constructor) {
      W << CombinatorFetch(constructor.get());
    } else if (is_type) {
      W << TypeFetch(is_bare_type, type, constructor_params_forward, template_brackets);
    }
    W << END << "\n\n";
  }
};

struct TypeDecl {
  bool is_bare_type;
  vk::tl::type *t;

  inline TypeDecl(vk::tl::type *t, bool is_bare_type) :
    is_bare_type(is_bare_type),
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << t->name << " ~~~~~~~~~ */\n" << NL;
    std::string struct_name = "t_" + cpp_tl_struct_name(t->name) + (is_bare_type ? "_$" : "");
    W << WrapperDecl(t->constructors[0], struct_name);
  }
};

struct TypeDef {
  bool is_bare_type;
  vk::tl::type *t;

  inline TypeDef(vk::tl::type *t, bool is_bare_type) :
    is_bare_type(is_bare_type),
    t(t) {}

  inline void compile(CodeGenerator &W) const {
    std::string struct_name = "t_" + cpp_tl_struct_name(t->name) + (is_bare_type ? "_$" : "");
    W << WrapperDef(t->constructors[0], struct_name);
  }
};

struct FunctionDecl {
  vk::tl::combinator *f;

  explicit FunctionDecl(vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    W << "/* ~~~~~~~~~ " << f->name << " ~~~~~~~~~ */\n" << NL;
    std::string struct_name = "f_" + cpp_tl_struct_name(f->name);
    W << "struct " + struct_name + " : tl_func_base " << BEGIN;
    for (const auto &arg : f->args) {
      if (arg->flags & FLAG_OPT_VAR) {
        W << "tl_exclamation_fetch_wrapper " << arg->name << ";" << NL;
      } else if (arg->var_num != -1) {
        W << "int " << arg->name << "{0};" << NL;
      }
    }
    W << "static std::unique_ptr<tl_func_base> store(const var& tl_object);" << NL;
    W << "var fetch();" << NL;
    W << "size_t get_sizeof() const final " << BEGIN
      << "return sizeof(" << struct_name << ");" << NL
      << END << NL;
    W << "const char *get_name() const final " << BEGIN
      << "return \"" << f->name << "\";" << NL
      << END << NL;
    W << END << ";\n" << NL;
  }
};

struct FunctionDef {
  vk::tl::combinator *f;

  explicit inline FunctionDef(vk::tl::combinator *f) :
    f(f) {}

  inline void compile(CodeGenerator &W) const {
    std::string struct_name = "f_" + cpp_tl_struct_name(f->name);
    W << "std::unique_ptr<tl_func_base> " << struct_name << "::store(const var& tl_object) " << BEGIN;
    if (DEBUG_MODE) {
      if (f->name != "memcache.get" && f->name != "memcache.set") {
        W << "fprintf(stderr, \"~~~ store fun (auto gen) : " << f->name << "\\n\");" << NL;
      }
    }
    W << "std::unique_ptr<" << struct_name << "> result_fetcher(new " << struct_name << "());" << NL;
    W << CombinatorStore(f);
    W << "return std::move(result_fetcher);" << NL;
    W << END << NL << NL;
    W << "var " << struct_name << "::fetch() " << BEGIN;
    if (DEBUG_MODE) {
      if (f->name != "memcache.get" && f->name != "memcache.set") {
        W << "fprintf(stderr, \"~~~ fetch fun (auto gen) : " << f->name << "\\n\");" << NL;
      }
    }
    W << CombinatorFetch(f);
    W << END << NL << NL;
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
  std::unordered_set<std::string> dependencies;
  std::string name;

  Module() = default;

  explicit Module(string name) :
    name(std::move(name)) {}

  void compile_tl_h_file(CodeGenerator &W) const {
    W << OpenFile(name + ".h", "tl");
    W << "#pragma once" << NL;
    W << ExternInclude("PHP/tl/tl_init.h");
    W << ExternInclude("tl/tl_const_vars.h");
    for (const auto &dep: dependencies) {
      W << Include("tl/" + dep + ".h");
    }
    W << NL;
    for (const auto &t : target_types) {
      for (const auto &constructor : t->constructors) {
        W << WrapperDecl(constructor, "c_" + cpp_tl_struct_name(constructor->name));
      }
    }
    for (const auto &t : target_types) {
      W << TypeDecl(t, false);
      if (is_type_dependent(t)) {
        W << TypeDef(t, false);
      }
      if (t->constructors_num == 1) {
        W << TypeDecl(t, true);
        if (is_type_dependent(t)) {
          W << TypeDef(t, true);
        }
      }
    }
    for (const auto &cell : cells) {
      W << cell;
    }
    for (const auto &t : target_types) {
      if (is_type_dependent(t)) {
        for (const auto &constructor : t->constructors) {
          W << WrapperDef(constructor, "c_" + cpp_tl_struct_name(constructor->name));
        }
      }
    }
    for (const auto &f : target_functions) {
      W << FunctionDecl(f);
    }
    W << CloseFile();
  }

  void compile_tl_cpp_file(CodeGenerator &W) const {
    W << OpenFile(name + ".cpp", "tl", false);
    W << ExternInclude("php_functions.h");
    W << Include("tl/" + name + ".h") << NL;
    for (const auto &t : target_types) {
      if (!is_type_dependent(t)) {
        W << TypeDef(t, false);
        if (t->constructors_num == 1) {
          W << TypeDef(t, true);
        }
        for (const auto &constructor : t->constructors) {
          W << WrapperDef(constructor, "c_" + cpp_tl_struct_name(constructor->name));
        }
      }
    }
    for (const auto &f: target_functions) {
      W << FunctionDef(f);
    }
    W << CloseFile();
  }

  inline void compile(CodeGenerator &W) const {
    compile_tl_h_file(W);
    compile_tl_cpp_file(W);
  }

  void add_obj(const std::unique_ptr<vk::tl::combinator> &f) {
    modules_with_functions.insert(name);
    target_functions.push_back(f.get());
    update_dependencies(f);
  }

  void add_obj(const std::unique_ptr<vk::tl::type> &t) {
    target_types.push_back(t.get());
    update_dependencies(t);
  }

  void add_obj(const Cell &cell) {
    cells.push_back(cell);
    update_dependencies(cell);
  }

  template<typename T>
  static std::string get_module(const T &named) {
    auto n = named->name.find('.');
    if (n == std::string::npos) {
      return "common";
    }
    return named->name.substr(0, n);
  }

  static std::string get_module(const Cell &cell) {
    return get_module(cell.owner);
  }

  template<typename T>
  static void add_to_module(const T &tl_obj) {
    auto module_name = get_module(tl_obj);
    ensure_existence(module_name).add_obj(tl_obj);
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
    std::string type_module = get_module(t);
    for (const auto &c : t->constructors) {
      update_dependencies(c);
      if (type_module != name) {
        ensure_existence(type_module).dependencies.insert(name);
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
      std::string expr_module = get_module(tl->types[as_type_expr->type_id]);
      if (name != expr_module) {
        dependencies.insert(expr_module);
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
    auto child_type = get_full_type_expr_str(child.get(), var_num_access);
    auto child_type_name = child_type.first;
    auto child_type_value = child_type.second;
    constructor_params.emplace_back(child_type_value);
    if (!child_type_name.empty()) {
      template_params.emplace_back(child_type_name);
    }
  }
  auto type_name = "t_" + (type->name != T_SHARP ? cpp_tl_struct_name(type->name) : cpp_tl_struct_name(tl->types[TL_INT]->name));
  auto bare_suf = ((as_type_expr->flags & FLAG_BARE) || type->name == T_SHARP ? "_$" : "");
  auto template_str = (!template_params.empty() ? "<" + join(template_params, ", ") + ">" : "");
  auto full_type_name = type_name + bare_suf + template_str;
  auto full_type_value = full_type_name + "(" + join(constructor_params, ", ") + ")";
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
  W << "array<var> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base> stored_fetcher) " << BEGIN
    << "tl_exclamation_fetch_wrapper X;" << NL
    << "X.fetcher = std::move(stored_fetcher);" << NL
    << "return t_ReqResult<tl_exclamation_fetch_wrapper>(std::move(X)).fetch();" << NL
    << END << NL << NL;
  W << "array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
  W << "void fill_tl_storers_ht() " << BEGIN;
  for (const auto &module_name : modules_with_functions) {
    for (const auto &f : modules[module_name].target_functions) {
      W << "gen$tl_storers_ht.set_value(" << register_tl_const_str(f->name) << ", " << "&f_" << cpp_tl_struct_name(f->name) << "::store, " << int_to_str(hash_tl_const_str(f->name)) << ");" << NL;
    }
  }
  W << END << NL;
  W << CloseFile();

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
  vector<int> const_string_shifts;
  compile_raw_data(W, const_string_shifts, tl_const_vars);
  W << "void tl_str_const_init() " << BEGIN;
  int i = 0;
  std::for_each(tl_const_vars.begin(), tl_const_vars.end(), [&](const std::string &s) {
    W << cpp_tl_const_str(s) << ".assign_raw (&raw[" << std::to_string(const_string_shifts[i++]) << "]);" << NL;
  });
  W << END;
  W << CloseFile();
}
} // namespace tl_gen

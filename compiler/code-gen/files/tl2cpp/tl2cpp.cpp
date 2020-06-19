#include "compiler/code-gen/files/tl2cpp/tl2cpp.h"

#include "common/tlo-parsing/flat-optimization.h"
#include "common/tlo-parsing/replace-anonymous-args.h"
#include "common/tlo-parsing/tl-scheme-final-check.h"

#include "compiler/code-gen/files/tl2cpp/tl-module.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"

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
namespace tl2cpp {

/* Разбиваем все комбинаторы и типы на модули, вместе с тем собирая зависимости каждого модуля.
 * */
void collect_target_objects() {
  auto should_exclude_tl_type = [](const std::unique_ptr<vk::tl::type> &t) {
    return CUSTOM_IMPL_TYPES.find(t->name) != CUSTOM_IMPL_TYPES.end()
           || t->name == "engine.Query";                        // это единственный тип с !X, надо бы его удалить из схемы
  };
  auto should_exclude_tl_function = [](const std::unique_ptr<vk::tl::combinator> &f) {
    return !G->env().get_gen_tl_internals() && f->is_internal_function();
  };

  for (const auto &e : tl->types) {
    const std::unique_ptr<vk::tl::type> &t = e.second;
    if (!should_exclude_tl_type(t)) {
      Module::add_to_module(t);
    }
  }

  for (const auto &e : tl->functions) {
    const std::unique_ptr<vk::tl::combinator> &f = e.second;
    if (!should_exclude_tl_function(f)) {
      Module::add_to_module(f);
    }
  }
}

void write_rpc_server_functions(CodeGenerator &W) {
  W << OpenFile("rpc_server_fetch_request.cpp", "tl", false);
  std::vector<vk::tl::combinator *> kphp_functions;
  IncludesCollector deps;
  deps.add_raw_filename_include("tl/common.h"); // чтобы компилилось, если нет ни одной @kphp функции
  for (const auto &item : tl->functions) {
    const auto &f = item.second;
    if (f->is_kphp_rpc_server_function()) {
      auto klass = get_php_class_of_tl_function(f.get());
      if (klass) { // Если !klass, то просто считаем что такой функции нет
        kphp_functions.emplace_back(f.get());
        deps.add_class_include(klass);
        deps.add_raw_filename_include("tl/" + Module::get_module_name(f->name) + ".h");
      }
    }
  }
  W << deps << NL;
  W << ExternInclude{"php_functions.h"} << NL;
  FunctionSignatureGenerator(W) << "class_instance<C$VK$TL$RpcFunction> f$rpc_server_fetch_request() " << BEGIN;
  W << "auto function_magic = static_cast<unsigned int>(f$fetch_int());" << NL;
  W << "switch(function_magic) " << BEGIN;
  for (const auto &f : kphp_functions) {
    W << fmt_format("case {:#010x}: ", static_cast<unsigned int>(f->id)) << BEGIN;
    W << get_php_runtime_type(f, true) << " request;" << NL
      << "request.alloc();" << NL
      << "CurrentRpcServerQuery::get().save(" << cpp_tl_struct_name("f_", f->name) << "::rpc_server_typed_fetch(request.get()));" << NL
      << "return request;" << NL
      << END << NL;
  }
  W << "default: " << BEGIN
    << "php_warning(\"Unexpected function magic on fetching request in rpc server: 0x%08x\", function_magic);" << NL
    << "return {};" << NL
    << END << NL;
  W << END << NL;
  W << END << NL;
  W << CloseFile();
}

void write_tl_query_handlers(CodeGenerator &W) {
  if (G->env().get_tl_schema_file().empty()) {
    return;
  }

  auto tl_ptr = vk::tl::parse_tlo(G->env().get_tl_schema_file().c_str(), false);
  kphp_error_return(tl_ptr.has_value(),
                    fmt_format("Error while reading tlo: {}", tl_ptr.error()));

  tl = tl_ptr.value().get();
  try {
    vk::tl::replace_anonymous_args(*tl);
    vk::tl::perform_flat_optimization(*tl);
    vk::tl::tl_scheme_final_check(*tl);
  } catch (const std::exception &ex) {
    kphp_error_return(false, ex.what());
  }
  collect_target_objects();
  for (const auto &e : modules) {
    const Module &module = e.second;
    W << module;
  }

  if (G->get_untyped_rpc_tl_used()) {
    W << OpenFile("tl_runtime_bindings.cpp", "tl", false);
    for (const auto &module_name : modules_with_functions) {
      W << Include("tl/" + module_name + ".h");
    }
    W << NL;
    // Указатель на gen$tl_fetch_wrapper прокидывается в рантайм и вызывается из fetch_function()
    // Это сделано для того, чтобы не тащить в рантайм t_ReqResult и все его зависимости
    FunctionSignatureGenerator(W) << "array<var> gen$tl_fetch_wrapper(std::unique_ptr<tl_func_base> stored_fetcher) " << BEGIN
                                  << "tl_exclamation_fetch_wrapper X(std::move(stored_fetcher));" << NL
                                  << "return t_ReqResult<tl_exclamation_fetch_wrapper, 0>(std::move(X)).fetch();" << NL
                                  << END << NL << NL;
    // Хэш таблица, содержащая все тл функции
    // Тоже прокидывается в рантайм
    W << "array<tl_storer_ptr> gen$tl_storers_ht;" << NL;
    FunctionSignatureGenerator(W) << "void fill_tl_storers_ht() " << BEGIN;
    for (const auto &module_name : modules_with_functions) {
      for (const auto &f : modules[module_name].target_functions) {
        W << "gen$tl_storers_ht.set_value(" << register_tl_const_str(f->name) << ", " << "&" << cpp_tl_struct_name("f_", f->name) << "::store, "
          << hash_tl_const_str(f->name) << ");" << NL;
      }
    }
    W << END << NL;
    W << CloseFile();
  }
  write_rpc_server_functions(W);
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
  FunctionSignatureGenerator(W) << "void tl_str_const_init() " << BEGIN;
  int i = 0;
  std::for_each(tl_const_vars.begin(), tl_const_vars.end(), [&](const std::string &s) {
    W << cpp_tl_const_str(s) << ".assign_raw (&raw[" << const_string_shifts[i++] << "]);" << NL;
  });
  W << END;
  W << CloseFile();
}
} // namespace tl_gen

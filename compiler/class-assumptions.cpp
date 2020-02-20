/*
 * Assumptions — предположения о том, какие переменные являются инстансами каких классов.
 * Нужен, чтобы стрелочным вызовам методов сопоставить реальные функции.
 * Так, $a->getValue() — нужно догадаться, что это getValue() именно класса A, и сопоставить Classes$A$$getValue().
 *
 * Сопоставление методов проходит в "Preprocess functions C pass" (try_set_func_id). Это ДО type inferring, и даже
 * до register variables, поэтому assumptions опираются исключительно на имена переменных.
 * Здесь реализован парсинг @return/@param/@type/@var у переменных и полей классов, а также проводится несложный анализ
 * кода функции, чтобы меньше phpdoc'ов писать. Так, "$a = new A; $a->...()" — он увидит, что вызван конструктор, и догадается.
 *
 * Если предсказатель ошибётся (или если неправильный phpdoc), то потом, постфактум, после type inferring проверится,
 * что ожидаемые предположения о типах и методах совпали с выведенными. Если нет — ошибка компиляции.
 * Т.е. assumptions — это ожидания, намерения; преданализ. Нужен для предварительной связки методов, чтобы работал inferring.
 * А собственно type inferring — это реальное использование в коде.
 *
 * Assumptions есть у функций (локальные переменные) и классов (поля класса).
 *
 * По коду, анализ assumption'ов вызывается только у тех функций и переменных, которые используются при -> операторе.
 */

#include "compiler/class-assumptions.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

vk::intrusive_ptr<Assumption> calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name);

vk::intrusive_ptr<Assumption> assumption_get_for_var(FunctionPtr f, vk::string_view var_name) {
  for (const auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      return name_and_a.second;
    }
  }

  return {};
}

vk::intrusive_ptr<Assumption> assumption_get_for_var(ClassPtr c, vk::string_view var_name) {
  for (const auto &name_and_a : c->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      return name_and_a.second;
    }
  }

  return {};
}

std::string assumption_debug(vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption) {
  return "$" + var_name + " is " + (assumption ? assumption->as_human_readable() : "null");
}

bool assumption_merge(vk::intrusive_ptr<Assumption> dst, const vk::intrusive_ptr<Assumption> &rhs) {
  auto merge_classes_lca = [](ClassPtr dst_class, ClassPtr rhs_class) -> ClassPtr {
    if (dst_class->is_parent_of(rhs_class)) {
      return dst_class;
    }
    if (rhs_class->is_interface() || dst_class->is_interface()) {
      if (auto common_interface = rhs_class->get_common_base_or_interface(dst_class)) {
        return common_interface;
      }
    }
    return ClassPtr{};
  };
  auto dst_as_instance = dst->try_as<AssumInstance>();
  auto rhs_as_instance = rhs->try_as<AssumInstance>();
  if (dst_as_instance && rhs_as_instance) {
    ClassPtr lca_class = merge_classes_lca(dst_as_instance->klass, rhs_as_instance->klass);
    if (lca_class) {
      dst_as_instance->klass = lca_class;
      return true;
    }
    return false;
  }

  auto dst_as_array = dst->try_as<AssumInstanceArray>();
  auto rhs_as_array = rhs->try_as<AssumInstanceArray>();
  if (dst_as_array && rhs_as_array) {
    ClassPtr lca_class = merge_classes_lca(dst_as_array->klass, rhs_as_array->klass);
    if (lca_class) {
      dst_as_array->klass = lca_class;
      return true;
    }
    return false;
  }

  auto dst_as_not_instance = dst->try_as<AssumNotInstance>();
  auto rhs_as_not_instance = rhs->try_as<AssumNotInstance>();
  if (dst_as_not_instance && rhs_as_not_instance) {
    return true;
  }

  return false;
}

void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption) {
  bool exists = false;

  for (auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      vk::intrusive_ptr<Assumption> &a = name_and_a.second;
      kphp_error(assumption_merge(a, assumption),
                 fmt_format("{}()::${} is both {} and {}\n", f->get_human_readable_name(), var_name, a->as_human_readable(), assumption->as_human_readable()));
      exists = true;
    }
  }

  if (!exists) {
    f->assumptions_for_vars.emplace_back(std::string(var_name), assumption);
    //printf("%s() %s\n", f->name.c_str(), assumption_debug(var_name, assumption).c_str());
  }
}

void assumption_add_for_return(FunctionPtr f, const vk::intrusive_ptr<Assumption> &assumption) {
  vk::intrusive_ptr<Assumption> &a = f->assumption_for_return;

  if (a) {
    kphp_error(assumption_merge(a, assumption),
               fmt_format("{}() returns both {} and {}\n", f->get_human_readable_name(), a->as_human_readable(), assumption->as_human_readable()));
  }

  f->assumption_for_return = assumption;
  //printf("%s() returns %s\n", f->name.c_str(), assumption_debug("return", assumption).c_str());
}

void assumption_add_for_var(ClassPtr c, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption) {
  bool exists = false;

  for (auto &name_and_a : c->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      vk::intrusive_ptr<Assumption> &a = name_and_a.second;
      kphp_error(assumption_merge(a, assumption),
                 fmt_format("{}::${} is both {} and {}\n", c->name, var_name, a->as_human_readable(), assumption->as_human_readable()));
      exists = true;
    }
  }

  if (!exists) {
    c->assumptions_for_vars.emplace_back(std::string(var_name), assumption);
    //printf("%s::%s\n", c->name.c_str(), assumption_debug(var_name, assumption).c_str());
  }
}

/*
 * Распарсив содержимое тега после @param/@return/@var в виде VertexPtr type_expr, пробуем найти там класс
 * Разбираем простые случаи: A|false, A[]; более сложные, когда класс внутри tuple — не интересуют
 */
vk::intrusive_ptr<Assumption> assumption_create_from_phpdoc(const PhpDocTagParseResult &result) {
  VertexPtr expr = result.type_expr;

  if (auto lca_rule = expr.try_as<op_type_expr_lca>()) {
    VertexRange or_rules = lca_rule->args();
    if (vk::any_of_equal(or_rules[1]->type_help, tp_False, tp_Null)) {
      expr = or_rules[0];      // из 'A|false', 'A[]|null' достаём 'A' / 'A[]'
    } else if (vk::any_of_equal(or_rules[0]->type_help, tp_False, tp_Null)) {
      expr = or_rules[1];      // аналогично, только null в начале
    }
  }

  if (expr->type_help == tp_Class) {
    return AssumInstance::create(expr.as<op_type_expr_class>()->class_ptr);
  } else if (expr->type_help == tp_array && expr.as<op_type_expr_type>()->args()[0]->type_help == tp_Class) {
    return AssumInstanceArray::create(expr.as<op_type_expr_type>()->args()[0].as<op_type_expr_class>()->class_ptr);
  }

  return AssumNotInstance::create();
}

/*
 * Если свойство класса является инстансом/массивом другого класса, оно обязательно должно быть помечено как phpdoc:
 * / ** @var A * / var $aInstance;
 * Распознаём такие phpdoc'и у объявления var'ов внутри классов.
 */
void analyze_phpdoc_of_class_field(ClassPtr c, vk::string_view var_name, const vk::string_view &phpdoc_str) {
  FunctionPtr holder_f = G->get_function("$" + replace_backslashes(c->name));
  if (auto parsed = phpdoc_find_tag(phpdoc_str, php_doc_tag::var, holder_f)) {
    if (parsed.var_name.empty() || var_name == parsed.var_name) {
      assumption_add_for_var(c, var_name, assumption_create_from_phpdoc(parsed));
    }
  }
}


/*
 * Из $a = ...rhs... определяем, что за тип присваивается $a
 */
void analyze_set_to_var(FunctionPtr f, vk::string_view var_name, const VertexPtr &rhs, size_t depth) {
  auto a = infer_class_of_expr(f, rhs, depth + 1);

  if (a && !a->try_as<AssumNotInstance>()) {
    assumption_add_for_var(f, var_name, a);
  }
}

/*
 * Из catch($ex) определяем, что $ex это Exception
 * Пользовательских классов исключений и наследования исключений у нас нет, и пока не планируется.
 */
void analyze_catch_of_var(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_try> root __attribute__((unused))) {
  assumption_add_for_var(f, var_name, AssumInstance::create(G->get_class("Exception")));
}

/*
 * Из foreach($arr as $a), если $arr это массив инстансов, делаем вывод, что $a это инстанс того же класса.
 */
static void analyze_foreach(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_foreach_param> root, size_t depth) {
  auto a = infer_class_of_expr(f, root->xs(), depth + 1);

  if (auto as_array = a->try_as<AssumInstanceArray>()) {
    assumption_add_for_var(f, var_name, AssumInstance::create(as_array->klass));
  }
}

/*
 * Из global $MC делаем вывод, что это Memcache.
 * Здесь зашиты global built-in переменные в нашем коде с заранее известными именами
 */
void analyze_global_var(FunctionPtr f, vk::string_view var_name) {
  if (vk::any_of_equal(var_name, "MC", "MC_Local", "MC_Ads", "MC_Log", "PMC", "mc_fast", "MC_Config", "MC_Stats")) {
    assumption_add_for_var(f, var_name, AssumInstance::create(G->get_memcache_class()));
  }
}


/*
 * Если есть запись $a->... (где $a это локальная переменная) то надо понять, что такое $a.
 * Собственно, этим и занимается эта функция: комплексно анализирует использования переменной и вызывает то, что выше.
 */
void calc_assumptions_for_var_internal(FunctionPtr f, vk::string_view var_name, VertexPtr root, size_t depth) {
  switch (root->type()) {
    case op_phpdoc_raw: { // assumptions вычисляются рано, и /** phpdoc'и с @var */ ещё не превращены в op_phpdoc_var
      for (const auto &parsed : phpdoc_find_tag_multi(root.as<op_phpdoc_raw>()->phpdoc_str, php_doc_tag::var, f)) {
        // умеем как в /** @var A $v */, так и в /** @var A */ $v ...
        const std::string &var_name = parsed.var_name.empty() ? root.as<op_phpdoc_raw>()->next_var_name : parsed.var_name;
        if (!var_name.empty()) {
          assumption_add_for_var(f, var_name, assumption_create_from_phpdoc(parsed));
        }
      }
      return;
    }

    case op_set: {
      auto set = root.as<op_set>();
      if (set->lhs()->type() == op_var && set->lhs()->get_string() == var_name) {
        analyze_set_to_var(f, var_name, set->rhs(), depth + 1);
      }
      return;
    }

    case op_try: {
      auto t = root.as<op_try>();
      if (t->exception()->type() == op_var && t->exception()->get_string() == var_name) {
        analyze_catch_of_var(f, var_name, t);
      }
      break;    // внутрь try зайти чтоб
    }

    case op_foreach_param: {
      auto foreach = root.as<op_foreach_param>();
      if (foreach->x()->type() == op_var && foreach->x()->get_string() == var_name) {
        analyze_foreach(f, var_name, foreach, depth + 1);
      }
      return;
    }

    case op_global: {
      auto global = root.as<op_global>();
      if (global->args()[0]->type() == op_var && global->args()[0]->get_string() == var_name) {
        analyze_global_var(f, var_name);
      }
      return;
    }

    default:
      break;
  }

  for (auto i : *root) {
    calc_assumptions_for_var_internal(f, var_name, i, depth + 1);
  }
}

/*
 * Для $a->... (где $a это аргумент функции) нужно уметь определять, что в функцию передано.
 * Это можно сделать либо через @param, либо через синтаксис function f(A $a) {
 * Вызывается единожды на функцию.
 */
void init_assumptions_for_arguments(FunctionPtr f, VertexAdaptor<op_function> root) {
  if (!f->phpdoc_str.empty()) {
    for (const auto &parsed : phpdoc_find_tag_multi(f->phpdoc_str, php_doc_tag::param, f)) {
      if (!parsed.var_name.empty()) {
        assumption_add_for_var(f, parsed.var_name, assumption_create_from_phpdoc(parsed));
      }
    }
  }

  VertexRange params = root->params()->args();
  for (auto i : params.get_reversed_range()) {
    VertexAdaptor<op_func_param> param = i.as<op_func_param>();
    if (!param->type_declaration.empty()) {
      auto result = phpdoc_parse_type_and_var_name(param->type_declaration, f);
      auto a = assumption_create_from_phpdoc(result);
      if (!a->try_as<AssumNotInstance>()) {
        assumption_add_for_var(f, param->var()->get_string(), a);
      }
    }
  }
}

bool parse_kphp_return_doc(FunctionPtr f) {
  auto type_str = phpdoc_find_tag_as_string(f->phpdoc_str, php_doc_tag::kphp_return);
  if (!type_str) {
    return true;
  }

  if (f->assumptions_inited_args != 2) {
    auto err_msg = "function: `" + f->name + "` was not instantiated yet, please add `@kphp-return` tag to function which called this function";
    kphp_error(false, err_msg.c_str());
    return false;
  }

  // нам нужно именно as_string и сами распарсим, т.к. "T $arg" нельзя превратить в дерево type_expr, класса T нет
  for (const auto &kphp_template_str : phpdoc_find_tag_as_string_multi(f->phpdoc_str, php_doc_tag::kphp_template)) {
    // @kphp-template $arg нам не подходит, мы ищем именно
    // @kphp-template T $arg (т.к. через T выражен @kphp-return)
    if (kphp_template_str.empty() || kphp_template_str[0] == '$') {
      continue;
    }
    auto vector_T_var_name = split_skipping_delimeters(kphp_template_str);    // ["T", "$arg"]
    kphp_assert(vector_T_var_name.size() > 1);
    std::string template_type_of_arg = vector_T_var_name[0];
    std::string template_arg_name = vector_T_var_name[1].substr(1);
    kphp_error_act(!template_arg_name.empty() && !template_type_of_arg.empty(), "wrong format of @kphp-template", return false);

    auto type_and_field_name = split_skipping_delimeters(*type_str, ":");
    kphp_error_act(vk::any_of_equal(type_and_field_name.size(), 1, 2), "wrong kphp-return supplied", return false);

    std::string T_type_name = std::move(type_and_field_name[0]);
    std::string field_name = type_and_field_name.size() > 1 ? std::move(type_and_field_name.back()) : "";

    bool return_type_eq_arg_type = template_type_of_arg == T_type_name;
    bool return_type_arr_of_arg_type = (template_type_of_arg + "[]") == T_type_name;
    bool return_type_element_of_arg_type = template_type_of_arg == (T_type_name + "[]");
    if (vk::string_view(field_name).ends_with("[]")) {
      field_name.erase(std::prev(field_name.end(), 2), field_name.end());
      return_type_arr_of_arg_type = true;
    }

    if (return_type_eq_arg_type || return_type_arr_of_arg_type || return_type_element_of_arg_type) {
      auto a = assumption_get_for_var(f, template_arg_name);

      if (!field_name.empty()) {
        ClassPtr klass = a->try_as<AssumInstance>() ? a->try_as<AssumInstance>()->klass : a->try_as<AssumInstanceArray>() ? a->try_as<AssumInstanceArray>()->klass : ClassPtr{};
        kphp_error_act(klass, fmt_format("try to get type of field({}) of non-instance", field_name), return false);
        a = calc_assumption_for_class_var(klass, field_name);
      }

      if (a->try_as<AssumInstance>() && return_type_arr_of_arg_type) {
        assumption_add_for_return(f, AssumInstanceArray::create(a->try_as<AssumInstance>()->klass));
      } else if (a->try_as<AssumInstanceArray>() && return_type_element_of_arg_type && field_name.empty()) {
        assumption_add_for_return(f, AssumInstance::create(a->try_as<AssumInstanceArray>()->klass));
      } else {
        assumption_add_for_return(f, a);
      }
      return true;
    }
  }

  kphp_error(false, "wrong kphp-return argument supplied");
  return false;
}

/*
 * Если $a = getSome(), $a->... , или getSome()->... , то нужно определить, что возвращает getSome().
 * Это можно сделать либо через @return, либо анализируя простые return'ы внутри функции.
 * Вызывается единожды на функцию.
 */
void init_assumptions_for_return(FunctionPtr f, VertexAdaptor<op_function> root) {
  assert (f->assumptions_inited_return == 1);
//  printf("[%d] init_assumptions_for_return of %s\n", get_thread_id(), f->name.c_str());

  if (!f->phpdoc_str.empty()) {
    if (auto parsed = phpdoc_find_tag(f->phpdoc_str, php_doc_tag::returns, f)) {
      assumption_add_for_return(f, assumption_create_from_phpdoc(parsed));
      return;
    }

    if (!parse_kphp_return_doc(f)) {
      return;
    }
  }

  for (auto i : *root->cmd()) {
    if (i->type() == op_return && i.as<op_return>()->has_expr()) {
      VertexPtr expr = i.as<op_return>()->expr();

      if (expr->type() == op_var && expr->get_string() == "this" && f->modifiers.is_instance()) {
        assumption_add_for_return(f, AssumInstance::create(f->class_id));  // return this
      } else if (auto call_vertex = expr.try_as<op_func_call>()) {
        if (call_vertex->str_val == ClassData::NAME_OF_CONSTRUCT && !call_vertex->args().empty()) {
          if (auto alloc = call_vertex->args()[0].try_as<op_alloc>()) {
            ClassPtr klass = G->get_class(alloc->allocated_class_name);
            kphp_assert(klass);
            assumption_add_for_return(f, AssumInstance::create(klass));        // return A
            return;
          }
        }
        if (auto fun = call_vertex->func_id) {
          auto return_a = calc_assumption_for_return(fun, call_vertex);
          if (return_a) {
            assumption_add_for_return(f, return_a);
          }
        }
      }
    }
  }
}

/*
 * Если известно, что $a это A, и мы видим запись $a->chat->..., нужно определить, что за chat.
 * Это можно определить только по phpdoc @var Chat перед 'public $chat' декларации свойства.
 * Единожды на класс вызывается эта функция, которая парсит все phpdoc'и ко всем var'ам.
 */
void init_assumptions_for_all_fields(ClassPtr c) {
  assert (c->assumptions_inited_vars == 1);
//  printf("[%d] init_assumptions_for_all_fields of %s\n", get_thread_id(), c->name.c_str());

  c->members.for_each([&](ClassMemberInstanceField &f) {
    analyze_phpdoc_of_class_field(c, f.local_name(), f.phpdoc_str);
  });
  c->members.for_each([&](ClassMemberStaticField &f) {
    analyze_phpdoc_of_class_field(c, f.local_name(), f.phpdoc_str);
  });
}


/*
 * Высокоуровневая функция, определяющая, что такое $a внутри f.
 * Включает кеширование повторных вызовов, init на f при первом вызове и пр.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth) {
  if (f->modifiers.is_instance() && var_name == "this") {
    return AssumInstance::create(f->class_id);
  }

  if (f->assumptions_inited_args == 0) {
    init_assumptions_for_arguments(f, f->root);
    f->assumptions_inited_args = 2;   // каждую функцию внутри обрабатывает 1 поток, нет возни с synchronize
  }

  const auto &existing = assumption_get_for_var(f, var_name);
  if (existing) {
    return existing;
  }


  calc_assumptions_for_var_internal(f, var_name, f->root->cmd(), depth + 1);

  if (f->type == FunctionData::func_global || f->type == FunctionData::func_switch) {
    if ((var_name.size() == 2 && var_name == "MC") || (var_name.size() == 3 && var_name == "PMC")) {
      analyze_global_var(f, var_name);
    }
  }

  const auto &calculated = assumption_get_for_var(f, var_name);
  if (calculated) {
    return calculated;
  }

  auto pos = var_name.find("$$");
  if (pos != std::string::npos) {   // static переменная класса, просто используется внутри функции
    if (auto of_class = G->get_class(replace_characters(std::string{var_name.substr(0, pos)}, '$', '\\'))) {
      return calc_assumption_for_class_var(of_class, var_name.substr(pos + 2));
    }
  }

  auto not_instance = AssumNotInstance::create();
  f->assumptions_for_vars.emplace_back(std::string{var_name}, not_instance);
  return not_instance;
}

/*
 * Высокоуровневая функция, определяющая, что возвращает f.
 * Включает кеширование повторных вызовов и init на f при первом вызове.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call) {
  if (f->is_extern()) {
    if (f->root->type_rule) {
      auto rule_meta = f->root->type_rule->rule();
      if (auto class_type_rule = rule_meta.try_as<op_type_expr_class>()) {
        return AssumInstance::create(class_type_rule->class_ptr);
      } else if (auto func_type_rule = rule_meta.try_as<op_type_expr_instance>()) {
        auto arg_ref = func_type_rule->expr().as<op_type_expr_arg_ref>();
        if (auto arg = GenTree::get_call_arg_ref(arg_ref, call)) {
          if (auto class_name = GenTree::get_constexpr_string(arg)) {
            if (auto klass = G->get_class(*class_name)) {
              return AssumInstance::create(klass);
            }
          }
        }
      }
    }
    return {};
  }

  if (f->is_constructor()) {
    return AssumInstance::create(f->class_id);
  }

  if (f->assumptions_inited_return == 0) {
    if (__sync_bool_compare_and_swap(&f->assumptions_inited_return, 0, 1)) {
      init_assumptions_for_return(f, f->root);
      __sync_synchronize();
      f->assumptions_inited_return = 2;
    }
  }
  while (f->assumptions_inited_return != 2) {
    __sync_synchronize();
  }

  return f->assumption_for_return;
}

/*
 * Высокоуровневая функция, определяющая, что такое ->nested внутри инстанса $a класса c.
 * Выключает кеширование и единождый вызов init на класс.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name) {
  if (c->assumptions_inited_vars == 0) {
    if (__sync_bool_compare_and_swap(&c->assumptions_inited_vars, 0, 1)) {
      init_assumptions_for_all_fields(c);   // как инстанс-переменные, так и статические
      __sync_synchronize();
      c->assumptions_inited_vars = 2;
    }
  }
  while (c->assumptions_inited_vars != 2) {
    __sync_synchronize();
  }

  return assumption_get_for_var(c, var_name);
}


// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————



vk::intrusive_ptr<Assumption> infer_from_var(FunctionPtr f,
                                             VertexAdaptor<op_var> var,
                                             size_t depth) {
  return calc_assumption_for_var(f, var->str_val, depth + 1);
}


vk::intrusive_ptr<Assumption> infer_from_call(FunctionPtr f,
                                              VertexAdaptor<op_func_call> call,
                                              size_t depth) {
  const std::string &fname = call->extra_type == op_ex_func_call_arrow
                             ? resolve_instance_func_name(f, call)
                             : get_full_static_member_name(f, call->str_val);

  const FunctionPtr ptr = G->get_function(fname);
  if (!ptr) {
    kphp_error(0, fmt_format("{}() is undefined, can not infer class", fname));
    return {};
  }

  // для built-in функций по типу array_pop/array_filter/etc на массиве инстансов
  if (auto common_rule = ptr->root->type_rule.try_as<op_common_type_rule>()) {
    auto rule = common_rule->rule();
    if (auto arg_ref = rule.try_as<op_type_expr_arg_ref>()) {     // array_values ($a ::: array) ::: ^1
      return infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), depth + 1);
    }
    if (auto index = rule.try_as<op_index>()) {         // array_shift (&$a ::: array) ::: ^1[]
      auto arr = index->array();
      if (auto arg_ref = arr.try_as<op_type_expr_arg_ref>()) {
        auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), depth + 1);
        if (auto arr_a = expr_a->try_as<AssumInstanceArray>()) {
          return AssumInstance::create(arr_a->klass);
        }
        return AssumNotInstance::create();
      }
    }
    if (auto array_rule = rule.try_as<op_type_expr_type>()) {  // create_vector ($n ::: int, $x ::: Any) ::: array <^2>
      if (array_rule->type_help == tp_array) {
        auto arr = array_rule->args()[0];
        if (auto arg_ref = arr.try_as<op_type_expr_arg_ref>()) {
          auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), depth + 1);
          if (auto inst_a = expr_a->try_as<AssumInstance>()) {
            return AssumInstanceArray::create(inst_a->klass);
          }
          return AssumNotInstance::create();
        }
      }
    }

  }

  return calc_assumption_for_return(ptr, call);
}

vk::intrusive_ptr<Assumption> infer_from_array(FunctionPtr f,
                                               VertexAdaptor<op_array> array,
                                               size_t depth) {
  kphp_assert(array);
  if (array->size() == 0) {
    return {};
  }

  ClassPtr klass;
  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    auto as_instance = infer_class_of_expr(f, v, depth + 1)->try_as<AssumInstance>();
    if (!as_instance) {
      return AssumNotInstance::create();
    }

    if (!klass) {
      klass = as_instance->klass;
    } else if (klass != as_instance->klass) {
      return AssumNotInstance::create();
    }
  }

  return AssumInstanceArray::create(klass);
}

vk::intrusive_ptr<Assumption> infer_from_instance_prop(FunctionPtr f,
                                                       VertexAdaptor<op_instance_prop> prop,
                                                       size_t depth) {
  auto lhs_a = infer_class_of_expr(f, prop->instance(), depth + 1)->try_as<AssumInstance>();
  if (!lhs_a->klass) {
    return AssumNotInstance::create();
  }

  ClassPtr lhs_class = lhs_a->klass;
  vk::intrusive_ptr<Assumption> res_assum;
  do {
    res_assum = calc_assumption_for_class_var(lhs_class, prop->str_val);
    lhs_class = lhs_class->parent_class;
  } while ((!res_assum || res_assum->try_as<AssumNotInstance>()) && lhs_class);

  return res_assum;
}

/*
 * Главная функция, вызывающаяся извне: возвращает assumption для любого выражения root внутри f.
 */
vk::intrusive_ptr<Assumption> infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth /*= 0*/) {
  if (depth > 1000) {
    return AssumNotInstance::create();
  }
  switch (root->type()) {
    case op_var:
      return infer_from_var(f, root.as<op_var>(), depth + 1);
    case op_instance_prop:
      return infer_from_instance_prop(f, root.as<op_instance_prop>(), depth + 1);
    case op_func_call:
      return infer_from_call(f, root.as<op_func_call>(), depth + 1);
    case op_index: {
      auto index = root.as<op_index>();
      if (index->has_key()) {
        auto expr_a = infer_class_of_expr(f, index->array(), depth + 1);
        if (auto arr_a = expr_a->try_as<AssumInstanceArray>()) {
          return AssumInstance::create(arr_a->klass);
        }
      }
      return AssumNotInstance::create();
    }
    case op_array:
      return infer_from_array(f, root.as<op_array>(), depth + 1);
    case op_conv_array:
    case op_conv_array_l:
      return infer_class_of_expr(f, root.as<meta_op_unary>()->expr(), depth + 1);
    case op_clone:
      return infer_class_of_expr(f, root.as<op_clone>()->expr(), depth + 1);
    case op_seq_rval:
      return infer_class_of_expr(f, root.as<op_seq_rval>()->back(), depth + 1);
    default:
      return AssumNotInstance::create();
  }
}

std::string AssumInstanceArray::as_human_readable() const {
  return klass->name + "[]";
}

std::string AssumInstance::as_human_readable() const {
  return klass->name;
}

std::string AssumNotInstance::as_human_readable() const {
  return "primitive";
}

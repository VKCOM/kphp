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
#include <thread>

#include "compiler/class-assumptions.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"
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
    auto common_bases = rhs_class->get_common_base_or_interface(dst_class);
    return common_bases.size() == 1 ? common_bases[0] : ClassPtr{};
  };
  auto dst_as_instance = dst.try_as<AssumInstance>();
  auto rhs_as_instance = rhs.try_as<AssumInstance>();
  if (dst_as_instance && rhs_as_instance) {
    ClassPtr lca_class = merge_classes_lca(dst_as_instance->klass, rhs_as_instance->klass);
    if (lca_class) {
      dst_as_instance->klass = lca_class;
      return true;
    }
    return false;
  }

  auto dst_as_array = dst.try_as<AssumArray>();
  auto rhs_as_array = rhs.try_as<AssumArray>();
  if (dst_as_array && rhs_as_array) {
    return assumption_merge(dst_as_array->inner, rhs_as_array->inner);
  }

  auto dst_as_tuple = dst.try_as<AssumTuple>();
  auto rhs_as_tuple = rhs.try_as<AssumTuple>();
  if (dst_as_tuple && rhs_as_tuple && dst_as_tuple->subkeys_assumptions.size() == rhs_as_tuple->subkeys_assumptions.size()) {
    bool ok = true;
    for (int i = 0; i < dst_as_tuple->subkeys_assumptions.size(); ++i) {
      ok &= assumption_merge(dst_as_tuple->subkeys_assumptions[i], rhs_as_tuple->subkeys_assumptions[i]);
    }
    return ok;
  }

  auto dst_as_shape = dst.try_as<AssumShape>();
  auto rhs_as_shape = rhs.try_as<AssumShape>();
  if (dst_as_shape && rhs_as_shape) {
    bool ok = true;
    for (const auto &it : dst_as_shape->subkeys_assumptions) {
      auto at_it = rhs_as_shape->subkeys_assumptions.find(it.first);
      if (at_it != rhs_as_shape->subkeys_assumptions.end()) {
        ok &= assumption_merge(it.second, at_it->second);
      }
    }
    return ok;
  }

  return dst->is_primitive() && rhs->is_primitive();
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
 * Разбираем известные случаи: A|null, A[], A[][]|false, tuple(A, B)
 */
vk::intrusive_ptr<Assumption> assumption_create_from_phpdoc(VertexPtr type_expr) {
  // из 'A|null', 'A[]|false', 'null|some_class' достаём 'A' / 'A[]' / 'some_class'
  bool or_false_flag = false;
  bool or_null_flag = false;
  if (auto lca_rule = type_expr.try_as<op_type_expr_lca>()) {
    VertexRange or_rules = lca_rule->args();
    auto upd_type_expr = [&](VertexPtr lhs, VertexPtr rhs) {
      or_false_flag = lhs->type_help == tp_False;
      or_null_flag = lhs->type_help == tp_Null;
      if (or_false_flag || or_null_flag || lhs->type() == op_type_expr_callable) {
        type_expr = rhs;
        return true;
      }
      return false;
    };
    if (!upd_type_expr(or_rules[0], or_rules[1])) {
      upd_type_expr(or_rules[1], or_rules[0]);
    }
  }

  switch (type_expr->type_help) {
    case tp_Class:
      return AssumInstance::create(type_expr.as<op_type_expr_class>()->class_ptr)->add_flags(or_null_flag, or_false_flag);

    case tp_array:
        return AssumArray::create(assumption_create_from_phpdoc(type_expr.as<op_type_expr_type>()->args()[0]))->add_flags(or_null_flag, or_false_flag);

    case tp_tuple: {
      decltype(AssumTuple::subkeys_assumptions) sub;
      for (VertexPtr sub_expr : *type_expr.as<op_type_expr_type>()) {
        sub.emplace_back(assumption_create_from_phpdoc(sub_expr));
      }
      return AssumTuple::create(std::move(sub))->add_flags(or_null_flag, or_false_flag);
    }

    case tp_shape: {
      decltype(AssumShape::subkeys_assumptions) sub;
      for (VertexPtr sub_expr : type_expr.as<op_type_expr_type>()->args()) {
        auto double_arrow = sub_expr.as<op_double_arrow>();
        sub.emplace(double_arrow->lhs()->get_string(), assumption_create_from_phpdoc(double_arrow->rhs()));
      }
      return AssumShape::create(std::move(sub))->add_flags(or_null_flag, or_false_flag);
    }

    default:
      return AssumNotInstance::create(type_expr->type_help)->add_flags(or_null_flag, or_false_flag);
  }
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
      assumption_add_for_var(c, var_name, assumption_create_from_phpdoc(parsed.type_expr));
    }
  }
}


/*
 * Из $a = ...rhs... определяем, что за тип присваивается $a
 */
void analyze_set_to_var(FunctionPtr f, vk::string_view var_name, const VertexPtr &rhs, size_t depth) {
  auto a = infer_class_of_expr(f, rhs, depth + 1);

  if (!a->is_primitive()) {
    assumption_add_for_var(f, var_name, a);
  }
}

/*
 * Из list(... $lhs_var ...) = ...rhs... определяем, что присваивается в $lhs_var (возможно, справа tuple/shape)
 */
void analyze_set_to_list_var(FunctionPtr f, vk::string_view var_name, VertexPtr index_key, const VertexPtr &rhs, size_t depth) {
  auto a = infer_class_of_expr(f, rhs, depth + 1);

  if (!a->get_subkey_by_index(index_key)->is_primitive()) {
    assumption_add_for_var(f, var_name, a->get_subkey_by_index(index_key));
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

  if (auto as_array = a.try_as<AssumArray>()) {
    assumption_add_for_var(f, var_name, as_array->inner);
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
          assumption_add_for_var(f, var_name, assumption_create_from_phpdoc(parsed.type_expr));
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

    case op_list: {
      auto list = root.as<op_list>();

      for (const auto x : list->list()) {
        const auto kv = x.as<op_list_keyval>();
        auto as_var = kv->var().try_as<op_var>();
        if (as_var && as_var->get_string() == var_name) {
          analyze_set_to_list_var(f, var_name, kv->key(), list->array(), depth + 1);
        }
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
        assumption_add_for_var(f, parsed.var_name, assumption_create_from_phpdoc(parsed.type_expr));
      }
    }
  }

  VertexRange params = root->params()->args();
  for (auto i : params.get_reversed_range()) {
    VertexAdaptor<op_func_param> param = i.as<op_func_param>();
    if (!param->type_declaration.empty()) {
      auto result = phpdoc_parse_type_and_var_name(param->type_declaration, f);
      auto a = assumption_create_from_phpdoc(result.type_expr);
      if (!a->is_primitive()) {
        assumption_add_for_var(f, param->var()->get_string(), a);
      }
    }
  }
}

bool parse_kphp_return_doc(FunctionPtr f) {
  auto type_str = phpdoc_find_tag_as_string(f->phpdoc_str, php_doc_tag::kphp_return);
  if (!type_str) {
    return false;
  }

  if (f->assumption_args_status != AssumptionStatus::initialized) {
    kphp_error(false, fmt_format("function {} was not instantiated yet, please add `@kphp-return` tag to function which called this function", f->get_human_readable_name()));
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
    auto template_type_of_arg = vector_T_var_name[0];
    auto template_arg_name = vector_T_var_name[1].substr(1);
    kphp_error_act(!template_arg_name.empty() && !template_type_of_arg.empty(), "wrong format of @kphp-template", return false);

    // `:` for getting type of field inside template type `@return T::data`
    auto type_and_field_name = split_skipping_delimeters(*type_str, ":");
    kphp_error_act(vk::any_of_equal(type_and_field_name.size(), 1, 2), "wrong kphp-return supplied", return false);

    auto T_type_name = type_and_field_name[0];
    auto field_name = type_and_field_name.size() > 1 ? type_and_field_name.back() : "";

    bool return_type_eq_arg_type = template_type_of_arg == T_type_name;
    // (x + "[]") == y; <=> y[-2:] == "[]" && x == y[0:-2]
    auto x_plus_brackets_equals_y = [](vk::string_view x, vk::string_view y) { return y.ends_with("[]") && x == y.substr(0, y.size() - 2); };
    bool return_type_arr_of_arg_type = x_plus_brackets_equals_y(template_type_of_arg, T_type_name);
    bool return_type_element_of_arg_type = x_plus_brackets_equals_y(T_type_name, template_type_of_arg);

    if (field_name.ends_with("[]")) {
      field_name = field_name.substr(0, field_name.size() - 2);
      return_type_arr_of_arg_type = true;
    }

    if (return_type_eq_arg_type || return_type_arr_of_arg_type || return_type_element_of_arg_type) {
      auto a = assumption_get_for_var(f, template_arg_name);

      if (!field_name.empty()) {
        ClassPtr klass =
          a.try_as<AssumInstance>() ? a.try_as<AssumInstance>()->klass :
          a.try_as<AssumArray>() && a.try_as<AssumArray>()->inner.try_as<AssumInstance>() ? a.try_as<AssumArray>()->inner.try_as<AssumInstance>()->klass :
          ClassPtr{};
        kphp_error_act(klass, fmt_format("try to get type of field({}) of non-instance", field_name), return false);
        a = calc_assumption_for_class_var(klass, field_name);
      }

      if (a.try_as<AssumInstance>() && return_type_arr_of_arg_type) {
        assumption_add_for_return(f, AssumArray::create(a.try_as<AssumInstance>()->klass));
      } else if (a.try_as<AssumArray>() && return_type_element_of_arg_type && field_name.empty()) {
        assumption_add_for_return(f, a.try_as<AssumArray>()->inner);
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
  assert (f->assumption_return_status == AssumptionStatus::processing);
//  printf("[%d] init_assumptions_for_return of %s\n", get_thread_id(), f->name.c_str());

  if (!f->phpdoc_str.empty()) {
    if (auto parsed = phpdoc_find_tag(f->phpdoc_str, php_doc_tag::returns, f)) {
      assumption_add_for_return(f, assumption_create_from_phpdoc(parsed.type_expr));
      return;
    }

    if (parse_kphp_return_doc(f)) {
      return;
    }
  }
  if (!f->return_typehint.empty()) {
    if (auto parsed = phpdoc_parse_type_and_var_name(f->return_typehint, f)) {
      if (parsed.type_expr->type() != op_type_expr_callable) {
        assumption_add_for_return(f, assumption_create_from_phpdoc(parsed.type_expr));
        return;
      }
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
        if (call_vertex->func_id) {
          assumption_add_for_return(f, calc_assumption_for_return(call_vertex->func_id, call_vertex));
        }
      }
    }
  }

  if (!f->assumption_for_return) {
    assumption_add_for_return(f, AssumNotInstance::create());
  }
}

/*
 * Если известно, что $a это A, и мы видим запись $a->chat->..., нужно определить, что за chat.
 * Это можно определить только по phpdoc @var Chat перед 'public $chat' декларации свойства.
 * Единожды на класс вызывается эта функция, которая парсит все phpdoc'и ко всем var'ам.
 */
void init_assumptions_for_all_fields(ClassPtr c) {
  assert (c->assumption_vars_status == AssumptionStatus::processing);
//  printf("[%d] init_assumptions_for_all_fields of %s\n", get_thread_id(), c->name.c_str());

  c->members.for_each([&](ClassMemberInstanceField &f) {
    analyze_phpdoc_of_class_field(c, f.local_name(), f.phpdoc_str);
  });
  c->members.for_each([&](ClassMemberStaticField &f) {
    analyze_phpdoc_of_class_field(c, f.local_name(), f.phpdoc_str);
  });
}

namespace {
template<class Atomic>
void assumption_busy_wait(Atomic &status_holder) noexcept {
  const auto start = std::chrono::steady_clock::now();
  while (status_holder != AssumptionStatus::initialized) {
    kphp_assert(std::chrono::steady_clock::now() - start < std::chrono::seconds{30});
    std::this_thread::sleep_for(std::chrono::nanoseconds{100});
  }
}
} // namespace


vk::intrusive_ptr<Assumption> calc_assumption_for_argument(FunctionPtr f, vk::string_view var_name) {
  if (f->modifiers.is_instance() && var_name == "this") {
    return AssumInstance::create(f->class_id);
  }

  auto expected = AssumptionStatus::unknown;
  if (f->assumption_args_status.compare_exchange_strong(expected, AssumptionStatus::processing)) {
    init_assumptions_for_arguments(f, f->root);
    f->assumption_args_status = AssumptionStatus::initialized;
  } else if (expected == AssumptionStatus::processing) {
    assumption_busy_wait(f->assumption_args_status);
  }

  return assumption_get_for_var(f, var_name);
}

/*
 * Высокоуровневая функция, определяющая, что такое $a внутри f.
 * Включает кеширование повторных вызовов, init на f при первом вызове и пр.
 * Всегда возвращает ненулевой assumption.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth) {
  if (auto assumption_from_argument = calc_assumption_for_argument(f, var_name)) {
    return assumption_from_argument;
  }

  calc_assumptions_for_var_internal(f, var_name, f->root->cmd(), depth + 1);

  if (f->is_main_function() || f->type == FunctionData::func_switch) {
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
      auto class_var_assum = calc_assumption_for_class_var(of_class, var_name.substr(pos + 2));
      return class_var_assum ?: AssumNotInstance::create();
    }
  }

  auto not_instance = AssumNotInstance::create();
  f->assumptions_for_vars.emplace_back(std::string{var_name}, not_instance);
  return not_instance;
}

/*
 * Высокоуровневая функция, определяющая, что возвращает f.
 * Включает кеширование повторных вызовов и init на f при первом вызове.
 * Всегда возвращает ненулевой assumption.
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
    return AssumNotInstance::create();
  }

  if (f->is_constructor()) {
    return AssumInstance::create(f->class_id);
  }

  auto expected = AssumptionStatus::unknown;
  if (f->assumption_return_status.compare_exchange_strong(expected, AssumptionStatus::processing)) {
    kphp_assert(f->assumption_return_processing_thread == std::thread::id{});
    f->assumption_return_processing_thread = std::this_thread::get_id();
    init_assumptions_for_return(f, f->root);
    f->assumption_return_status = AssumptionStatus::initialized;
  } else if (expected == AssumptionStatus::processing) {
    if (f->assumption_return_processing_thread == std::this_thread::get_id()) {
      // Проверяем рекурсию в рамках одного потока, поэтому порядок работы с атомарными операциями не так важен.
      // Если другой поток изменит assumption_return_status но не успеет выставить assumption_return_processing_thread,
      // то мы все равно не попадем в этот if, так как assumption_return_processing_thread будет иметь дефолтное значение
      return AssumNotInstance::create();
    }
    assumption_busy_wait(f->assumption_return_status);
  }

  return f->assumption_for_return;
}

/*
 * Высокоуровневая функция, определяющая, что такое ->nested внутри инстанса $a класса c.
 * Выключает кеширование и единождый вызов init на класс.
 * Может вернуть нулевой assumption, если переменной нет.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name) {
  auto expected = AssumptionStatus::unknown;
  if (c->assumption_vars_status.compare_exchange_strong(expected, AssumptionStatus::processing)) {
    init_assumptions_for_all_fields(c);   // как инстанс-переменные, так и статические
    c->assumption_vars_status = AssumptionStatus::initialized;
  } else if (expected == AssumptionStatus::processing) {
    assumption_busy_wait(c->assumption_vars_status);
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
    return AssumNotInstance::create();
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
        if (auto arr_a = expr_a.try_as<AssumArray>()) {
          return arr_a->inner;
        }
        return AssumNotInstance::create();
      }
    }
    if (auto array_rule = rule.try_as<op_type_expr_type>()) {  // create_vector ($n ::: int, $x ::: Any) ::: array <^2>
      if (array_rule->type_help == tp_array) {
        auto arr = array_rule->args()[0];
        if (auto arg_ref = arr.try_as<op_type_expr_arg_ref>()) {
          auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), depth + 1);
          if (auto inst_a = expr_a.try_as<AssumInstance>()) {
            return AssumArray::create(inst_a->klass);
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
  if (array->size() == 0) {
    return AssumNotInstance::create();
  }

  ClassPtr klass;
  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    auto as_instance = infer_class_of_expr(f, v, depth + 1).try_as<AssumInstance>();
    if (!as_instance) {
      return AssumNotInstance::create();
    }

    if (!klass) {
      klass = as_instance->klass;
    } else if (klass != as_instance->klass) {
      return AssumNotInstance::create();
    }
  }

  return AssumArray::create(klass);
}

vk::intrusive_ptr<Assumption> infer_from_tuple(FunctionPtr f,
                                               VertexAdaptor<op_tuple> tuple,
                                               size_t depth) {
  decltype(AssumTuple::subkeys_assumptions) sub;
  for (auto sub_expr : *tuple) {
    sub.emplace_back(infer_class_of_expr(f, sub_expr, depth + 1));
  }
  return AssumTuple::create(std::move(sub));
}

vk::intrusive_ptr<Assumption> infer_from_shape(FunctionPtr f,
                                               VertexAdaptor<op_shape> shape,
                                               size_t depth) {
  decltype(AssumShape::subkeys_assumptions) sub;
  for (auto sub_expr : shape->args()) {
    auto double_arrow = sub_expr.as<op_double_arrow>();
    sub.emplace(GenTree::get_actual_value(double_arrow->lhs())->get_string(), infer_class_of_expr(f, double_arrow->rhs(), depth + 1));
  }
  return AssumShape::create(std::move(sub));
}

vk::intrusive_ptr<Assumption> infer_from_pair_vertex_merge(FunctionPtr f, Location location, VertexPtr a, VertexPtr b, size_t depth) {
  auto a_assumption = infer_class_of_expr(f, a, depth + 1);
  auto b_assumption = infer_class_of_expr(f, b, depth + 1);
  if (!a_assumption->is_primitive() && !b_assumption->is_primitive()) {
    std::swap(*stage::get_location_ptr(), location);
    kphp_error(assumption_merge(a_assumption, b_assumption),
               fmt_format("result of operation is both {} and {}\n",
                          a_assumption->as_human_readable(), b_assumption->as_human_readable()));
    std::swap(*stage::get_location_ptr(), location);
  }
  return a_assumption->is_primitive() ? b_assumption : a_assumption;
}

vk::intrusive_ptr<Assumption> infer_from_ternary(FunctionPtr f, VertexAdaptor<op_ternary> ternary, size_t depth) {
  // для поддержки оператора $x = $y ?: $z;
  // он разворачивается в $x = bool($tmp_var = $y) ? move($tmp_var) : $z;
  if (auto move_true_expr = ternary->true_expr().try_as<op_move>()) {
    if (auto tmp_var = move_true_expr->expr().try_as<op_var>()) {
      calc_assumptions_for_var_internal(f, tmp_var->get_string(), ternary->cond(), depth + 1);
    }
  }

  return infer_from_pair_vertex_merge(f, ternary->get_location(), ternary->true_expr(), ternary->false_expr(), depth);
}

vk::intrusive_ptr<Assumption> infer_from_instance_prop(FunctionPtr f,
                                                       VertexAdaptor<op_instance_prop> prop,
                                                       size_t depth) {
  auto lhs_a = infer_class_of_expr(f, prop->instance(), depth + 1).try_as<AssumInstance>();
  if (!lhs_a || !lhs_a->klass) {
    return AssumNotInstance::create();
  }

  ClassPtr lhs_class = lhs_a->klass;
  vk::intrusive_ptr<Assumption> res_assum;
  do {
    res_assum = calc_assumption_for_class_var(lhs_class, prop->str_val);
    lhs_class = lhs_class->parent_class;
  } while ((!res_assum || res_assum.try_as<AssumNotInstance>()) && lhs_class);

  return res_assum ?: AssumNotInstance::create();
}

/*
 * Главная функция, вызывающаяся извне: возвращает assumption для любого выражения root внутри f.
 * Гарантированно возвращает не null внутри (если ничего адекватного — AssumNotInstance, но не null)
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
        return infer_class_of_expr(f, index->array(), depth + 1)->get_subkey_by_index(index->key());
      }
      return AssumNotInstance::create();
    }
    case op_array:
      return infer_from_array(f, root.as<op_array>(), depth + 1);
    case op_tuple:
      return infer_from_tuple(f, root.as<op_tuple>(), depth + 1);
    case op_shape:
      return infer_from_shape(f, root.as<op_shape>(), depth + 1);
    case op_ternary:
      return infer_from_ternary(f, root.as<op_ternary>(), depth + 1);
    case op_null_coalesce: {
      auto null_coalesce = root.as<op_null_coalesce>();
      return infer_from_pair_vertex_merge(f, null_coalesce->get_location(),
                                          null_coalesce->lhs(), null_coalesce->rhs(), depth + 1);
    }
    case op_conv_array:
    case op_conv_array_l:
      return infer_class_of_expr(f, root.as<meta_op_unary>()->expr(), depth + 1);
    case op_clone:
      return infer_class_of_expr(f, root.as<op_clone>()->expr(), depth + 1);
    case op_seq_rval:
      return infer_class_of_expr(f, root.as<op_seq_rval>()->back(), depth + 1);
    case op_move:
      return infer_class_of_expr(f, root.as<op_move>()->expr(), depth + 1);
    default:
      return AssumNotInstance::create();
  }
}


// ——————————————


std::string AssumArray::as_human_readable() const {
  return inner->as_human_readable() + "[]";
}

std::string AssumInstance::as_human_readable() const {
  return klass->name;
}

std::string AssumNotInstance::as_human_readable() const {
  return type != tp_Any ? ptype_name(type) : "primitive";
}

std::string AssumTuple::as_human_readable() const {
  std::string r = "tuple(";
  for (const auto &a_sub : subkeys_assumptions) {
    r += a_sub->as_human_readable() + ",";
  }
  r[r.size() - 1] = ')';
  return r;
}

std::string AssumShape::as_human_readable() const {
  std::string r = "shape(";
  for (const auto &a_sub : subkeys_assumptions) {
    r += a_sub.first + ":" + a_sub.second->as_human_readable() + ",";
  }
  r[r.size() - 1] = ')';
  return r;
}


bool AssumArray::is_primitive() const {
  return inner->is_primitive();
}

bool AssumInstance::is_primitive() const {
  return false;
}

bool AssumNotInstance::is_primitive() const {
  return true;
}

bool AssumTuple::is_primitive() const {
  return false;
}

bool AssumShape::is_primitive() const {
  return false;
}


const TypeData *AssumArray::get_type_data() const {
  return TypeData::create_type_data(inner->get_type_data(), or_null_, or_false_);
}

const TypeData *AssumInstance::get_type_data() const {
  return klass->type_data;
}

const TypeData *AssumNotInstance::get_type_data() const {
  auto res = TypeData::get_type(type)->clone();
  if (or_null_) {
    res->set_or_null_flag();
  }
  if (or_false_) {
    res->set_or_false_flag();
  }
  return res;
}

const TypeData *AssumTuple::get_type_data() const {
  std::vector<const TypeData *> subkeys_values;
  std::transform(subkeys_assumptions.begin(), subkeys_assumptions.end(), std::back_inserter(subkeys_values),
                 [](const vk::intrusive_ptr<Assumption> &a) { return a->get_type_data(); });
  return TypeData::create_type_data(subkeys_values, or_null_, or_false_);
}

const TypeData *AssumShape::get_type_data() const {
  std::map<std::string, const TypeData *> subkeys_values;
  for (const auto &sub : subkeys_assumptions) {
    subkeys_values.emplace(sub.first, sub.second->get_type_data());
  }
  return TypeData::create_type_data(subkeys_values, or_null_, or_false_);
}


vk::intrusive_ptr<Assumption> AssumArray::get_subkey_by_index(VertexPtr index_key __attribute__ ((unused))) const {
  return inner;
}

vk::intrusive_ptr<Assumption> AssumInstance::get_subkey_by_index(VertexPtr index_key __attribute__ ((unused))) const {
  return AssumNotInstance::create();
}

vk::intrusive_ptr<Assumption> AssumNotInstance::get_subkey_by_index(VertexPtr index_key __attribute__ ((unused))) const {
  return AssumNotInstance::create();
}

vk::intrusive_ptr<Assumption> AssumTuple::get_subkey_by_index(VertexPtr index_key) const {
  if (auto as_int_index = GenTree::get_actual_value(index_key).try_as<op_int_const>()) {
    int int_index = parse_int_from_string(as_int_index);
    if (int_index >= 0 && int_index < subkeys_assumptions.size()) {
      return subkeys_assumptions[int_index];
    }
  }
  return AssumNotInstance::create();
}

vk::intrusive_ptr<Assumption> AssumShape::get_subkey_by_index(VertexPtr index_key) const {
  if (vk::any_of_equal(GenTree::get_actual_value(index_key)->type(), op_int_const, op_string)) {
    const auto &string_index = GenTree::get_actual_value(index_key)->get_string();
    auto at_index = subkeys_assumptions.find(string_index);
    if (at_index != subkeys_assumptions.end()) {
      return at_index->second;
    }
  }
  return AssumNotInstance::create();
}

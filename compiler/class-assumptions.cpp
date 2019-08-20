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

AssumType calc_assumption_for_class_var(ClassPtr c, const std::string &var_name, ClassPtr &out_class);

AssumType assumption_get_for_var(FunctionPtr f, const std::string &var_name, ClassPtr &out_class) {
  for (const auto &a : f->assumptions_for_vars) {
    if (a.var_name == var_name) {
      out_class = a.klass;
      return a.assum_type;
    }
  }

  return assum_unknown;
}

AssumType assumption_get_for_var(ClassPtr c, const std::string &var_name, ClassPtr &out_class) {
  for (const auto &a : c->assumptions_for_vars) {
    if (a.var_name == var_name) {
      out_class = a.klass;
      return a.assum_type;
    }
  }

  return assum_unknown;
}

std::string assumption_debug(const Assumption &assumption) {
  switch (assumption.assum_type) {
    case assum_not_instance:
      return "$" + assumption.var_name + " is not instance";
    case assum_instance:
      return "$" + assumption.var_name + " is " + (assumption.klass ? assumption.klass->name : "null");
    case assum_instance_array:
      return "$" + assumption.var_name + " is " + (assumption.klass ? assumption.klass->name : "null") + "[]";
    default:
      return "$" + assumption.var_name + " unknown";
  }
}

void assumption_add_for_var(FunctionPtr f, AssumType assum, const std::string &var_name, ClassPtr klass) {
  bool exists = false;

  for (auto &a : f->assumptions_for_vars) {
    if (a.var_name == var_name) {
      bool ok = a.assum_type == assum;
      if (ok && (a.klass != klass && !a.klass->is_parent_of(klass))) {
        ok = false;
        if (klass->is_interface() || a.klass->is_interface()) {
          if (auto common_interface = klass->get_common_base_or_interface(a.klass)) {
            a.klass = common_interface;
            ok = true;
          }
        }
      }
      kphp_error(ok, format("%s()::$%s is both %s and %s\n", f->get_human_readable_name().c_str(), var_name.c_str(),
                            a.klass ? a.klass->name.c_str() : "[primitive]",
                            klass ? klass->name.c_str() : "[primitive]"));
      exists = true;
    }
  }

  if (!exists) {
    f->assumptions_for_vars.emplace_back(assum, var_name, klass);
    //printf("%s() %s\n", f->name.c_str(), assumption_debug(f->assumptions_for_vars.back()).c_str());
  }
}

void assumption_add_for_return(FunctionPtr f, AssumType assum, ClassPtr klass) {
  const Assumption &a = f->assumption_for_return;

  if (a.assum_type != assum_unknown) {
    kphp_error(a.assum_type == assum && a.klass == klass,
               format("%s() returns both %s and %s\n", f->get_human_readable_name().c_str(),
                       a.klass ? a.klass->name.c_str() : "[primitive]",
                       klass ? klass->name.c_str() : "[primitive]"));
  }

  f->assumption_for_return = Assumption(assum, "", klass);
  //printf("%s() returns %s\n", f->name.c_str(), assumption_debug(f->assumption_for_return).c_str());
}

void assumption_add_for_var(ClassPtr c, AssumType assum, const std::string &var_name, ClassPtr klass) {
  bool exists = false;

  for (const auto &a : c->assumptions_for_vars) {
    if (a.var_name == var_name) {
      kphp_error(a.assum_type == assum && a.klass == klass,
                 format("%s::$%s is both %s and %s\n", var_name.c_str(), c->name.c_str(), a.klass->name.c_str(), klass->name.c_str()));
      exists = true;
    }
  }

  if (!exists) {
    c->assumptions_for_vars.emplace_back(assum, var_name, klass);
    //printf("%s::%s\n", c->name.c_str(), assumption_debug(c->assumptions_for_vars.back()).c_str());
  }
}

/*
 * Анализ phpdoc-типов вида 'A', '\VK\A[]', 'int' и т.п. на предмет того, что это класс.
 * Если это класс, то он не может быть смешан с другими классами и типами (парсинг ругнётся) (но A|false это ок).
 */
AssumType parse_phpdoc_classname(const std::string &type_str, ClassPtr &out_klass, FunctionPtr current_function) {
  // в нашей репке очень много уже невалидных phpdoc'ов по типу '@param [type] $actor', на которых парсинг будет ругаться
  // до появления инстансов они не парсились, т.к. @kphp- не содержат; а сейчас начнут выдавать ошибку,
  // если внутри таких функций есть ->обращения; надо бы эти phpdoc'и все править, но пока что с этим как-то жить
  // и вот чтобы не реагировать на ошибки, то не парсим очередной @param, если там, видимо, нет имени класса
  bool seems_classname_inside = false;
  for (char c : type_str) {
    if (c >= 'A' && c <= 'Z') {
      seems_classname_inside = true;
      break;
    }
  }
  if (!seems_classname_inside && type_str != "self") {
    return assum_not_instance;
  }

  VertexPtr type_rule = phpdoc_parse_type(type_str, current_function);
  if (!type_rule) {      // если всё-таки проник некорректный phpdoc-тип внутрь, чтоб не закрешилось
    return assum_not_instance;
  }

  // phpdoc-тип в виде строки сейчас представлен в виде дерева vertex'ов; допускаем 'A', 'A[]', 'A|false', 'false|\A'
  // другие, более сложные, по типу '(A|int)', не разбираем и считаем assum_not_instance
  if (auto lca_rule = type_rule.try_as<op_type_expr_lca>()) {
    VertexRange or_rules = lca_rule->args();
    if (or_rules[1]->type_help == tp_False || or_rules[1]->type_help == tp_bool) {
      type_rule = or_rules[0];      // из 'A|false', 'A[]|false', 'A|bool' достаём 'A' / 'A[]'
    } else if (or_rules[0]->type_help == tp_False || or_rules[0]->type_help == tp_bool) {
      type_rule = or_rules[1];      // аналогично, только false в начале
    }
  }

  if (type_rule->type_help == tp_Class) {
    out_klass = type_rule.as<op_type_expr_class>()->class_ptr;
    return assum_instance;
  } else if (type_rule->type_help == tp_array && type_rule.as<op_type_expr_type>()->args()[0]->type_help == tp_Class) {
    out_klass = type_rule.as<op_type_expr_type>()->args()[0].as<op_type_expr_class>()->class_ptr;
    return assum_instance_array;
  }

  return assum_not_instance;
}


/*
 * Анализ следующего паттерна. Если есть переменная $a, то при присваивании ей можно написать
 * / ** @var A * / $a = ...;
 * Т.е. имя класса есть, а название переменной может отсутствовать (но это в контексте конкретной переменной, это ок).
 */
void analyze_phpdoc_with_type(FunctionPtr f, const std::string &var_name, const vk::string_view &phpdoc_str) {
  int param_i = 0;
  std::string param_var_name, type_str;
  while (PhpDocTypeRuleParser::find_tag_in_phpdoc(phpdoc_str, php_doc_tag::var, param_var_name, type_str, param_i++)) {
    if (!param_var_name.empty() || !var_name.empty()) {
      ClassPtr klass;
      AssumType assum = parse_phpdoc_classname(type_str, klass, f);
      assumption_add_for_var(f, assum, param_var_name.empty() ? var_name : param_var_name, klass);
    }
  }
}

/*
 * Если свойство класса является инстансом/массивом другого класса, оно обязательно должно быть помечено как phpdoc:
 * / ** @var A * / var $aInstance;
 * Распознаём такие phpdoc'и у объявления var'ов внутри классов.
 */
void analyze_phpdoc_with_type(ClassPtr c, const std::string &var_name, const vk::string_view &phpdoc_str) {
  std::string type_str, param_var_name;
  ClassPtr klass;
  if (PhpDocTypeRuleParser::find_tag_in_phpdoc(phpdoc_str, php_doc_tag::var, param_var_name, type_str)) {
    AssumType assum = parse_phpdoc_classname(type_str, klass, c->file_id->main_function);

    if (klass && (param_var_name.empty() || var_name == param_var_name)) {
      assumption_add_for_var(c, assum, var_name, klass);
    }
  }
}


/*
 * Из $a = ...rhs... определяем, что за тип присваивается $a
 */
void analyze_set_to_var(FunctionPtr f, const std::string &var_name, const VertexPtr &rhs, size_t depth) {
  ClassPtr klass;
  AssumType assum = infer_class_of_expr(f, rhs, klass, depth + 1);

  if (assum != assum_unknown && klass) {
    assumption_add_for_var(f, assum, var_name, klass);
  }
}

/*
 * Из catch($ex) определяем, что $ex это Exception
 * Пользовательских классов исключений и наследования исключений у нас нет, и пока не планируется.
 */
void analyze_catch_of_var(FunctionPtr f, const std::string &var_name, VertexAdaptor<op_try> root __attribute__((unused))) {
  assumption_add_for_var(f, assum_instance, var_name, G->get_class("Exception"));
}

/*
 * Из foreach($arr as $a), если $arr это массив инстансов, делаем вывод, что $a это инстанс того же класса.
 */
static void analyze_foreach(FunctionPtr f, const std::string &var_name, VertexAdaptor<op_foreach_param> root, size_t depth) {
  ClassPtr klass;
  AssumType iter_assum = infer_class_of_expr(f, root->xs(), klass, depth + 1);

  if (iter_assum == assum_instance_array) {
    assumption_add_for_var(f, assum_instance, var_name, klass);
  }
}

/*
 * Из global $MC делаем вывод, что это Memcache.
 * Здесь зашиты global built-in переменные в нашем коде с заранее известными именами
 */
void analyze_global_var(FunctionPtr f, const std::string &var_name) {
  if (var_name == "MC" || var_name == "MC_Local" || var_name == "MC_Ads" || var_name == "MC_Log"
      || var_name == "PMC" || var_name == "mc_fast" || var_name == "MC_Config" || var_name == "MC_Stats") {
    assumption_add_for_var(f, assum_instance, var_name, G->get_memcache_class());
  }
}


/*
 * Если есть запись $a->... (где $a это локальная переменная) то надо понять, что такое $a.
 * Собственно, этим и занимается эта функция: комплексно анализирует использования переменной и вызывает то, что выше.
 */
void calc_assumptions_for_var_internal(FunctionPtr f, const std::string &var_name, VertexPtr root, size_t depth) {
  switch (root->type()) {
    case op_set: {
      auto set = root.as<op_set>();
      if (set->lhs()->type() == op_var && set->lhs()->get_string() == var_name) {
        if (!set->phpdoc_str.empty()) {
          analyze_phpdoc_with_type(f, var_name, set->phpdoc_str);
        } else {
          analyze_set_to_var(f, var_name, set->rhs(), depth + 1);
        }
      }
      return;
    }

    case op_list: {
      if (!root.as<op_list>()->phpdoc_str.empty()) {
        analyze_phpdoc_with_type(f, std::string(), root.as<op_list>()->phpdoc_str);
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
    int param_i = 0;
    std::string param_var_name, type_str;
    while (PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_str, php_doc_tag::param, param_var_name, type_str, param_i++)) {
      if (!param_var_name.empty() && !type_str.empty()) {
        ClassPtr klass;
        AssumType assum = parse_phpdoc_classname(type_str, klass, f);
        assumption_add_for_var(f, assum, param_var_name, klass);
      }
    }
  }

  VertexRange params = root->params()->args();
  for (auto i : params.get_reversed_range()) {
    VertexAdaptor<op_func_param> param = i.as<op_func_param>();
    if (!param->type_declaration.empty() && param->type_declaration != "array") {
      ClassPtr klass = G->get_class(resolve_uses(f, param->type_declaration, '\\'));
      kphp_error(klass, format("Class %s near $%s does not exist or never created", param->type_declaration.c_str(), param->var()->get_c_string()));
      assumption_add_for_var(f, assum_instance, param->var()->get_string(), klass);
    }
  }
}

bool parse_kphp_return_doc(FunctionPtr f) {
  std::string type_str, dummy;

  if (!PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_str, php_doc_tag::kphp_return, dummy, type_str)) {
    return true;
  }

  if (f->assumptions_inited_args != 2) {
    auto err_msg = "function: `" + f->name + "` was not instantiated yet, please add `@kphp-return` tag to function which called this function";
    kphp_error(false, err_msg.c_str());
    return false;
  }

  int param_i = 0;
  std::string template_type_of_arg;
  std::string template_arg_name;
  while (true) {
    bool kphp_template_found = PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_str, php_doc_tag::kphp_template,
                                                                        template_arg_name, template_type_of_arg, param_i++);
    if (!kphp_template_found) {
      break;
    }

    if (!template_arg_name.empty() && !template_type_of_arg.empty()) {
      auto type_and_field_name = split_skipping_delimeters(type_str, ":");
      kphp_error_act(vk::any_of_equal(type_and_field_name.size(), 1, 2), "wrong kphp-return supplied", return false);

      type_str = std::move(type_and_field_name[0]);
      std::string field_name = type_and_field_name.size() > 1 ? std::move(type_and_field_name.back()) : "";

      bool return_type_eq_arg_type = template_type_of_arg == type_str;
      bool return_type_arr_of_arg_type = (template_type_of_arg + "[]") == type_str;
      bool return_type_element_of_arg_type = template_type_of_arg == (type_str + "[]");
      if (vk::string_view(field_name).ends_with("[]")) {
        field_name.erase(std::prev(field_name.end(), 2), field_name.end());
        return_type_arr_of_arg_type = true;
      }

      if (return_type_eq_arg_type || return_type_arr_of_arg_type || return_type_element_of_arg_type) {
        ClassPtr klass;
        AssumType assum = assumption_get_for_var(f, template_arg_name, klass);

        if (!field_name.empty()) {
          kphp_error_act(klass, format("try to get type of field(%s) of non-instance", field_name.c_str()), return false);
          assum = calc_assumption_for_class_var(klass, field_name, klass);
        }

        if (assum == assum_instance && return_type_arr_of_arg_type) {
          assumption_add_for_return(f, assum_instance_array, klass);
        } else if (assum == assum_instance_array && return_type_element_of_arg_type && field_name.empty()) {
          assumption_add_for_return(f, assum_instance, klass);
        } else {
          assumption_add_for_return(f, assum, klass);
        }
        return true;
      }
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
    std::string type_str, dummy;
    if (PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_str, php_doc_tag::returns, dummy, type_str)) {
      ClassPtr klass;
      AssumType assum = parse_phpdoc_classname(type_str, klass, f);
      assumption_add_for_return(f, assum, klass);       // 'self' тоже работает
      return;
    }

    if (!parse_kphp_return_doc(f)) {
      return;
    }
  }

  for (auto i : *root->cmd()) {
    if (i->type() == op_return && i.as<op_return>()->has_expr()) {
      VertexPtr expr = i.as<op_return>()->expr();

      if (expr->type() == op_constructor_call) {
        ClassPtr klass = G->get_class(expr->get_string());
        kphp_assert(klass);
        assumption_add_for_return(f, assum_instance, klass);        // return A
      } else if (expr->type() == op_var && expr->get_string() == "this" && f->modifiers.is_instance()) {
        assumption_add_for_return(f, assum_instance, f->class_id);  // return this
      } else if (auto call_vertex = expr.try_as<op_func_call>()) {
        if (auto fun = call_vertex->func_id) {
          ClassPtr klass;
          calc_assumption_for_return(fun, call_vertex, klass);
          if (klass) {
            assumption_add_for_return(f, assum_instance, klass);
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
void init_assumptions_for_all_vars(ClassPtr c) {
  assert (c->assumptions_inited_vars == 1);
//  printf("[%d] init_assumptions_for_all_vars of %s\n", get_thread_id(), c->name.c_str());

  c->members.for_each([&](ClassMemberInstanceField &f) {
    if (!f.phpdoc_str.empty()) {
      analyze_phpdoc_with_type(c, f.local_name(), f.phpdoc_str);
    }
  });
  c->members.for_each([&](ClassMemberStaticField &f) {
    if (!f.phpdoc_str.empty()) {
      analyze_phpdoc_with_type(c, f.local_name(), f.phpdoc_str);
    }
  });
}


/*
 * Высокоуровневая функция, определяющая, что такое $a внутри f.
 * Включает кеширование повторных вызовов, init на f при первом вызове и пр.
 */
AssumType calc_assumption_for_var(FunctionPtr f, const std::string &var_name, ClassPtr &out_class, size_t depth) {
  if (f->modifiers.is_instance() && var_name.size() == 4 && var_name == "this") {
    out_class = f->class_id;
    return assum_instance;
  }

  if (f->assumptions_inited_args == 0) {
    init_assumptions_for_arguments(f, f->root);
    f->assumptions_inited_args = 2;   // каждую функцию внутри обрабатывает 1 поток, нет возни с synchronize
  }

  AssumType existing = assumption_get_for_var(f, var_name, out_class);
  if (existing != assum_unknown) {
    return existing;
  }


  calc_assumptions_for_var_internal(f, var_name, f->root->cmd(), depth + 1);

  if (f->type == FunctionData::func_global || f->type == FunctionData::func_switch) {
    if ((var_name.size() == 2 && var_name == "MC") || (var_name.size() == 3 && var_name == "PMC")) {
      analyze_global_var(f, var_name);
    }
  }

  AssumType calculated = assumption_get_for_var(f, var_name, out_class);
  if (calculated != assum_unknown) {
    return calculated;
  }

  auto pos = var_name.find("$$");
  if (pos != std::string::npos) {   // static переменная класса, просто используется внутри функции
    ClassPtr of_class = G->get_class(replace_characters(var_name.substr(0, pos), '$', '\\'));
    if (of_class) {
      return calc_assumption_for_class_var(of_class, var_name.substr(pos + 2), out_class);
    }
  }

  f->assumptions_for_vars.emplace_back(assum_not_instance, var_name, ClassPtr());
  return assum_not_instance;
}

/*
 * Высокоуровневая функция, определяющая, что возвращает f.
 * Включает кеширование повторных вызовов и init на f при первом вызове.
 */
AssumType calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call, ClassPtr &out_class) {
  if (f->is_extern()) {
    if (f->root->type_rule) {
      auto rule_meta = f->root->type_rule->rule();
      if (auto class_type_rule = rule_meta.try_as<op_type_expr_class>()) {
        out_class = class_type_rule->class_ptr;
        return assum_instance;
      } else if (auto func_type_rule = rule_meta.try_as<op_type_expr_instance>()) {
        auto arg_ref = func_type_rule->expr().as<op_type_expr_arg_ref>();
        if (auto arg = GenTree::get_call_arg_ref(arg_ref, call)) {
          if (auto class_name = GenTree::get_constexpr_string(arg)) {
            if (auto klass = G->get_class(*class_name)) {
              out_class = klass;
              return assum_instance;
            }
          }
        }
      }
    }
    return assum_unknown;
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

  out_class = f->assumption_for_return.klass;
  return f->assumption_for_return.assum_type;
}

/*
 * Высокоуровневая функция, определяющая, что такое ->nested внутри инстанса $a класса c.
 * Выключает кеширование и единождый вызов init на класс.
 */
AssumType calc_assumption_for_class_var(ClassPtr c, const std::string &var_name, ClassPtr &out_class) {
  if (c->assumptions_inited_vars == 0) {
    if (__sync_bool_compare_and_swap(&c->assumptions_inited_vars, 0, 1)) {
      init_assumptions_for_all_vars(c);   // как инстанс-переменные, так и статические
      __sync_synchronize();
      c->assumptions_inited_vars = 2;
    }
  }
  while (c->assumptions_inited_vars != 2) {
    __sync_synchronize();
  }

  return assumption_get_for_var(c, var_name, out_class);
}


// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————


inline AssumType infer_from_ctor(FunctionPtr f __attribute__ ((unused)),
                                 VertexAdaptor<op_constructor_call> call,
                                 ClassPtr &out_class) {
  out_class = G->get_class(call->get_string());   // call->get_string() это полное имя класса после new
  return assum_instance;
}


inline AssumType infer_from_var(FunctionPtr f,
                                VertexAdaptor<op_var> var,
                                ClassPtr &out_class,
                                size_t depth) {
  return calc_assumption_for_var(f, var->str_val, out_class, depth + 1);
}


inline AssumType infer_from_call(FunctionPtr f,
                                 VertexAdaptor<op_func_call> call,
                                 ClassPtr &out_class,
                                 size_t depth) {
  const std::string &fname = call->extra_type == op_ex_func_call_arrow
                             ? resolve_instance_func_name(f, call)
                             : get_full_static_member_name(f, call->str_val);

  const FunctionPtr ptr = G->get_function(fname);
  if (!ptr) {
    kphp_error(0, format("%s() is undefined, can not infer class", fname.c_str()));
    return assum_unknown;
  }

  // для built-in функций по типу array_pop/array_filter/etc на массиве инстансов
  if (auto common_rule = ptr->root->type_rule.try_as<op_common_type_rule>()) {
    auto rule = common_rule->rule();
    if (auto arg_ref = rule.try_as<op_type_expr_arg_ref>()) {     // array_values ($a ::: array) ::: ^1
      return infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), out_class, depth + 1);
    }
    if (auto index = rule.try_as<op_index>()) {         // array_shift (&$a ::: array) ::: ^1[]
      auto arr = index->array();
      if (auto arg_ref = arr.try_as<op_type_expr_arg_ref>()) {
        AssumType result = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), out_class, depth + 1);
        if (result == assum_instance_array) {
          result = assum_instance;
        } else if (result == assum_instance) {
          out_class = {};
          result = assum_not_instance;
        }
        return result;
      }
    }
    if (auto array_rule = rule.try_as<op_type_expr_type>()) {  // create_vector ($n ::: int, $x ::: Any) ::: array <^2>
      if (array_rule->type_help == tp_array) {
        auto arr = array_rule->args()[0];
        if (auto arg_ref = arr.try_as<op_type_expr_arg_ref>()) {
          AssumType result = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref, call), out_class, depth + 1);
          if (result == assum_instance) {
            result = assum_instance_array;
          } else if (result == assum_instance_array) {
            out_class = {};
            result = assum_not_instance;
          }
          return result;
        }
      }
    }

  }

  return calc_assumption_for_return(ptr, call, out_class);
}

inline AssumType infer_from_array(FunctionPtr f,
                                  VertexAdaptor<op_array> array,
                                  ClassPtr &out_class,
                                  size_t depth) {
  kphp_assert(array);
  if (array->size() == 0) {
    return assum_unknown;
  }

  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    ClassPtr inferred_class;
    AssumType assum = infer_class_of_expr(f, v, inferred_class, depth + 1);
    if (assum != assum_instance) {
      return assum_not_instance;
    }

    if (!out_class) {
      out_class = inferred_class;
    } else if (out_class->name != inferred_class->name) {
      return assum_not_instance;
    }
  }

  return assum_instance_array;
}

AssumType infer_from_instance_prop(FunctionPtr f,
                                   VertexAdaptor<op_instance_prop> prop,
                                   ClassPtr &out_class,
                                   size_t depth) {
  ClassPtr lhs_class;
  AssumType lhs_assum = infer_class_of_expr(f, prop->instance(), lhs_class, depth + 1);

  if (lhs_assum != assum_instance) {
    return assum_not_instance;
  }

  AssumType res_assum;
  do {
    res_assum = calc_assumption_for_class_var(lhs_class, prop->str_val, out_class);
    lhs_class = lhs_class->parent_class;
  } while (vk::any_of_equal(res_assum, assum_unknown, assum_not_instance) && lhs_class);

  return res_assum;
}

/*
 * Главная функция, вызывающаяся извне: возвращает assumption для любого выражения root внутри f.
 */
AssumType infer_class_of_expr(FunctionPtr f, VertexPtr root, ClassPtr &out_class, size_t depth /*= 0*/) {
  if (depth > 10000) {
    return assum_not_instance;
  }
  switch (root->type()) {
    case op_constructor_call:
      return infer_from_ctor(f, root.as<op_constructor_call>(), out_class);
    case op_var:
      return infer_from_var(f, root.as<op_var>(), out_class, depth + 1);
    case op_instance_prop:
      return infer_from_instance_prop(f, root.as<op_instance_prop>(), out_class, depth + 1);
    case op_func_call:
      return infer_from_call(f, root.as<op_func_call>(), out_class, depth + 1);
    case op_index: {
      auto index = root.as<op_index>();
      if (index->has_key() && assum_instance_array == infer_class_of_expr(f, index->array(), out_class, depth + 1)) {
        return assum_instance;
      } else {
        return assum_not_instance;
      }
    }
    case op_array:
      return infer_from_array(f, root.as<op_array>(), out_class, depth + 1);
    case op_conv_array:
    case op_conv_array_l:
      return infer_class_of_expr(f, root.as<meta_op_unary>()->expr(), out_class, depth + 1);
    case op_clone:
      return infer_class_of_expr(f, root.as<op_clone>()->expr(), out_class, depth + 1);
    case op_seq_rval:
      return infer_class_of_expr(f, root.as<op_seq_rval>()->back(), out_class, depth + 1);
    default:
      return assum_not_instance;
  }
}


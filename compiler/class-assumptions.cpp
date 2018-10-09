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
#include "compiler/phpdoc.h"

static const std::string VAR_NAME_RETURN = "$return";


AssumType assumption_get(FunctionPtr f, const std::string &var_name, ClassPtr &out_class) {
  for (std::vector<Assumption>::const_iterator i = f->assumptions.begin(); i != f->assumptions.end(); ++i) {
    if (i->var_name == var_name) {
      out_class = i->klass;
      return i->assum_type;
    }
  }

  return assum_unknown;
}

AssumType assumption_get(ClassPtr c, const std::string &var_name, ClassPtr &out_class) {
  for (std::vector<Assumption>::const_iterator i = c->assumptions.begin(); i != c->assumptions.end(); ++i) {
    if (i->var_name == var_name) {
      out_class = i->klass;
      return i->assum_type;
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

void assumption_add(FunctionPtr f, AssumType assum, const std::string &var_name, ClassPtr klass) {
  bool exists = false;

  for (std::vector<Assumption>::iterator i = f->assumptions.begin(); i != f->assumptions.end(); ++i) {
    if (i->var_name == var_name) {
      if (i->assum_type != assum || i->klass != klass)
        kphp_error(false, dl_pstr("Assumption for %s()::$%s is ambigulous\n", f->name.c_str(), var_name.c_str()));
      exists = true;
    }
  }

  if (!exists) {
    f->assumptions.push_back(Assumption(assum, var_name, klass));
//    printf("%s() %s\n", f->name.c_str(), assumption_debug(f->assumptions.back()).c_str());
  }
}

void assumption_add(FunctionPtr f, AssumType assum, const std::string &var_name, const std::string &class_name) {
  ClassPtr klass = class_name.empty() ? ClassPtr() : G->get_class(resolve_uses(f, class_name, '\\'));
  kphp_error(klass,
             dl_pstr("Class %s (used in %s()) does not exist or never initialized", class_name.c_str(), f->name.c_str()));
  if (klass) {
    assumption_add(f, assum, var_name, klass);
  }
}

void assumption_add(ClassPtr c, AssumType assum, const std::string &var_name, ClassPtr klass) {
  bool exists = false;

  for (std::vector<Assumption>::iterator i = c->assumptions.begin(); i != c->assumptions.end(); ++i) {
    if (i->var_name == var_name) {
      if (i->assum_type != assum || i->klass != klass)
        kphp_error(false, dl_pstr("Assumption for %s::$%s is ambigulous\n", var_name.c_str(), c->name.c_str()));
      exists = true;
    }
  }

  if (!exists) {
    c->assumptions.push_back(Assumption(assum, var_name, klass));
//    printf("%s::%s\n", c->name.c_str(), assumption_debug(c->assumptions.back()).c_str());
  }
}

void assumption_add(ClassPtr c, AssumType assum, const std::string &var_name, const std::string &class_name) {
  ClassPtr klass = class_name.empty() ? ClassPtr() : G->get_class(resolve_uses(c->init_function, class_name, '\\'));
  kphp_error(klass,
             dl_pstr("Class %s (used in %s) does not exist or never initialized", class_name.c_str(), c->name.c_str()));
  if (klass) {
    assumption_add(c, assum, var_name, klass);
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
  for (int i = 0; i < type_str.size(); ++i) {
    if (type_str[i] >= 'A' && type_str[i] <= 'Z') {
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

  // phpdoc-тип в виде строки сейчас представлен в виде дерева vertex'ов; допускаем 'A', 'A[]', 'A|false'
  // другие, более сложные, по типу '(A|int)', не разбираем и считаем assum_not_instance
  if (unlikely(type_rule->type() == op_type_rule_func && type_rule->ith(1)->type_help == tp_False)) {
    type_rule = type_rule->ith(0);  // из 'A|false', 'A[]|false' достаём 'A' / 'A[]'
  }

  if (type_rule->type_help == tp_Class) {
    out_klass = type_rule.as<op_class_type_rule>()->class_ptr;
    return assum_instance;
  } else if (type_rule->type_help == tp_array && type_rule->ith(0)->type_help == tp_Class) {
    out_klass = type_rule->ith(0).as<op_class_type_rule>()->class_ptr;
    return assum_instance_array;
  }

  return assum_not_instance;
}


/*
 * Анализ следующего паттерна. Если есть переменная $a, то при присваивании ей можно написать
 * / ** @var A * / $a = ...;
 * Т.е. имя класса есть, а название переменной может отсутствовать (но это в контексте конкретной переменной, это ок).
 */
void analyze_phpdoc_with_type(FunctionPtr f, const std::string &var_name, const Token *phpdoc_token) {
  std::string type_str, param_var_name;
  ClassPtr klass;
  if (PhpDocTypeRuleParser::find_tag_in_phpdoc(phpdoc_token->str_val, php_doc_tag::var, param_var_name, type_str)) {
    AssumType assum = parse_phpdoc_classname(type_str, klass, f);

    if (!param_var_name.empty() || !var_name.empty()) {
      assumption_add(f, assum, param_var_name.empty() ? var_name : param_var_name, klass);
    }
  }
}

/*
 * Если свойство класса является инстансом/массивом другого класса, оно обязательно должно быть помечено как phpdoc:
 * / ** @var A * / var $aInstance;
 * Распознаём такие phpdoc'и у объявления var'ов внутри классов.
 */
void analyze_phpdoc_with_type(ClassPtr c, const std::string &var_name, const Token *phpdoc_token) {
  std::string type_str, param_var_name;
  ClassPtr klass;
  if (PhpDocTypeRuleParser::find_tag_in_phpdoc(phpdoc_token->str_val, php_doc_tag::var, param_var_name, type_str)) {
    AssumType assum = parse_phpdoc_classname(type_str, klass, c->init_function);

    if (klass && (param_var_name.empty() || var_name == param_var_name)) {
      assumption_add(c, assum, var_name, klass);
    }
  }
}


/*
 * Из $a = ...rhs... определяем, что за тип присваивается $a
 */
void analyze_set_to_var(FunctionPtr f, const std::string &var_name, const VertexPtr &rhs) {
  ClassPtr klass;
  AssumType assum = infer_class_of_expr(f, rhs, klass);

  if (assum != assum_unknown && klass) {
    assumption_add(f, assum, var_name, klass);
  }
}

/*
 * Из catch($ex) определяем, что $ex это Exception
 * Пользовательских классов исключений и наследования исключений у нас нет, и пока не планируется.
 */
void analyze_catch_of_var(FunctionPtr f, const std::string &var_name, VertexAdaptor<op_try> root __attribute__((unused))) {
  assumption_add(f, assum_instance, var_name, "\\Exception");
}

/*
 * Из foreach($arr as $a), если $arr это массив инстансов, делаем вывод, что $a это инстанс того же класса.
 */
void analyze_foreach(FunctionPtr f, const std::string &var_name, VertexAdaptor<op_foreach_param> root) {
  ClassPtr klass;
  AssumType iter_assum = infer_class_of_expr(f, root->ith(0), klass);

  if (iter_assum == assum_instance_array) {
    assumption_add(f, assum_instance, var_name, klass);
  }
}

/*
 * Из global $MC делаем вывод, что это Memcache.
 * Здесь зашиты global built-in переменные в нашем коде с заранее известными именами
 */
void analyze_global_var(FunctionPtr f, const std::string &var_name) {
  if (var_name == "MC" || var_name == "MC_Local" || var_name == "MC2" || var_name == "MC_Ads"
      || var_name == "PMC" || var_name == "mc_fast" || var_name == "MC_Config" || var_name == "MC_Stats"
      || var_name == "MC_Log") {
    assumption_add(f, assum_instance, var_name, "\\Memcache");
  }
}


/*
 * Если есть запись $a->... (где $a это локальная переменная) то надо понять, что такое $a.
 * Собственно, этим и занимается эта функция: комплексно анализирует использования переменной и вызывает то, что выше.
 */
void calc_assumptions_for_var_internal(FunctionPtr f, const std::string &var_name, VertexPtr root) {
  switch (root->type()) {
    case op_set:
      if (root->ith(0)->type() == op_var && root->ith(0)->get_string() == var_name) {
        if (root.as<op_set>()->phpdoc_token) {
          analyze_phpdoc_with_type(f, var_name, root.as<op_set>()->phpdoc_token);
        } else {
          analyze_set_to_var(f, var_name, root->ith(1));
        }
      }
      return;

    case op_list:
      if (root.as<op_list>()->phpdoc_token) {
        analyze_phpdoc_with_type(f, std::string(), root.as<op_list>()->phpdoc_token);
      }
      return;

    case op_try:
      if (root->size() > 1 && root->ith(1)->type() == op_var && root->ith(1)->get_string() == var_name) {
        analyze_catch_of_var(f, var_name, root);
      }
      break;    // внутрь try зайти чтоб

    case op_foreach_param:
      if (root->ith(1)->type() == op_var && root->ith(1)->get_string() == var_name) {
        analyze_foreach(f, var_name, root);
      }
      return;

    case op_global:
      if (root->ith(0)->type() == op_var && root->ith(0)->get_string() == var_name) {
        analyze_global_var(f, var_name);
      }
      return;

    default:
      break;
  }

  for (auto i : *root) {
    calc_assumptions_for_var_internal(f, var_name, i);
  }
}

/*
 * Для $a->... (где $a это аргумент функции) нужно уметь определять, что в функцию передано.
 * Это можно сделать либо через @param, либо через синтаксис function f(A $a) {
 * Вызывается единожды на функцию.
 */
void init_assumptions_for_arguments(FunctionPtr f, VertexAdaptor<op_function> root) {
  if (f->phpdoc_token != nullptr) {
    int param_i = 0;
    std::string param_var_name, type_str;
    ClassPtr klass;
    while (PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_token->str_val, php_doc_tag::param, param_var_name, type_str, param_i)) {
      if (!param_var_name.empty() && !type_str.empty()) {
        AssumType assum = parse_phpdoc_classname(type_str, klass, f);
        assumption_add(f, assum, param_var_name, klass);
      }
      param_i++;
    }
  }

  VertexRange params = root->params().as<op_func_param_list>()->args();
  for (auto i : params.get_reversed_range()) {
    VertexAdaptor<op_func_param> param = i;
    if (!param->type_declaration.empty()) {
      assumption_add(f, assum_instance, param->var()->get_string(), param->type_declaration);
    }
  }
}

/*
 * Если $a = getSome(), $a->... , или getSome()->... , то нужно определить, что возвращает getSome().
 * Это можно сделать либо через @return, либо анализируя простые return'ы внутри функции.
 * Вызывается единожды на функцию.
 */
void init_assumptions_for_return(FunctionPtr f, VertexAdaptor<op_function> root) {
  assert (f->assumptions_inited_return == 1);
//  printf("[%d] init_assumptions_for_return of %s\n", get_thread_id(), f->name.c_str());

  if (f->phpdoc_token != nullptr) {
    std::string type_str, dummy;
    ClassPtr klass;
    if (PhpDocTypeRuleParser::find_tag_in_phpdoc(f->phpdoc_token->str_val, php_doc_tag::returns, dummy, type_str)) {
      AssumType assum = parse_phpdoc_classname(type_str, klass, f);
      assumption_add(f, assum, VAR_NAME_RETURN, klass);       // 'self' тоже работает
      return;
    }
  }

  for (auto i : *root->cmd()) {
    if (i->type() == op_return) {
      VertexPtr expr = i.as<op_return>()->expr();

      if (expr->type() == op_constructor_call) {
        assumption_add(f, assum_instance, VAR_NAME_RETURN, expr->get_string());   // return A
      } else if (expr->type() == op_var && expr->get_string() == "this" && f->is_instance_function()) {
        assumption_add(f, assum_instance, VAR_NAME_RETURN, f->class_name);        // return this
      }
    }
  }
}

/*
 * Если известно, что $a это A, и мы видим запись $a->chat->..., нужно определить, что за chat.
 * Это можно определить только по phpdoc @var Chat перед 'public $chat' декларации свойства.
 * Единожды на класс вызывается эта функция, которая парсит все phpdoc'и ко всем var'ам.
 */
void init_assumptions_for_all_vars(ClassPtr c, VertexAdaptor<op_class> root __attribute__((unused))) {
  assert (c->assumptions_inited_vars == 1);
//  printf("[%d] init_assumptions_for_all_vars of %s\n", get_thread_id(), c->name.c_str());

  for (vector<VarPtr>::const_iterator var = c->vars.begin(); var != c->vars.end(); ++var) {
    if ((*var)->phpdoc_token) {
      analyze_phpdoc_with_type(c, (*var)->name, (*var)->phpdoc_token);
    }
  }
}


/*
 * Высокоуровневая функция, определяющая, что такое $a внутри f.
 * Включает кеширование повторных вызовов, init на f при первом вызове и пр.
 */
AssumType calc_assumption_for_var(FunctionPtr f, const std::string &var_name, ClassPtr &out_class) {
  if (f->is_instance_function() && var_name.size() == 4 && var_name == "this") {
    out_class = f->class_id;
    return assum_instance;
  }

  if (f->assumptions_inited_args == 0) {
    init_assumptions_for_arguments(f, f->root);
    f->assumptions_inited_args = 2;   // каждую функцию внутри обрабатывает 1 поток, нет возни с synchronize
  }

  AssumType existing = assumption_get(f, var_name, out_class);
  if (existing != assum_unknown) {
    return existing;
  }


  calc_assumptions_for_var_internal(f, var_name, f->root.as<op_function>()->cmd());

  if (f->type() == FunctionData::func_global || f->type() == FunctionData::func_switch) {
    if ((var_name.size() == 2 && var_name == "MC") || (var_name.size() == 3 && var_name == "PMC")) {
      assumption_add(f, assum_instance, var_name, "\\Memcache");
    }
  }

  AssumType calculated = assumption_get(f, var_name, out_class);
  if (calculated != assum_unknown) {
    return calculated;
  }

  f->assumptions.push_back(Assumption(assum_not_instance, var_name, ClassPtr()));
  return assum_not_instance;
}

/*
 * Высокоуровневая функция, определяющая, что возвращает f.
 * Включает кеширование повторных вызовов и init на f при первом вызове.
 */
AssumType calc_assumption_for_return(FunctionPtr f, ClassPtr &out_class) {
  if (f->type() == FunctionData::func_extern) {
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

  return assumption_get(f, VAR_NAME_RETURN, out_class);
}

/*
 * Высокоуровневая функция, определяющая, что такое ->nested внутри инстанса $a класса c.
 * Выключает кеширование и единождый вызов init на класс.
 */
AssumType calc_assumption_for_class_var(ClassPtr c, const std::string &var_name, ClassPtr &out_class) {
  if (c->assumptions_inited_vars == 0) {
    if (__sync_bool_compare_and_swap(&c->assumptions_inited_vars, 0, 1)) {
      init_assumptions_for_all_vars(c, c->root);
      __sync_synchronize();
      c->assumptions_inited_vars = 2;
    }
  }
  while (c->assumptions_inited_vars != 2) {
    __sync_synchronize();
  }

  return assumption_get(c, var_name, out_class);
}


// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————


inline AssumType infer_from_ctor(FunctionPtr f,
                                 VertexAdaptor<op_constructor_call> call,
                                 ClassPtr &out_class) {
  if (likely(!call->type_help)) {
    out_class = G->get_class(resolve_uses(f, call->str_val, '\\'));
    return assum_instance;
  }

  if (call->type_help == tp_MC) {
    out_class = G->get_class("Memcache");
  } else if (call->type_help == tp_Exception) {
    out_class = G->get_class("Exception");
  } else {
    kphp_error(0, "Unknown type_help");
  }
  return assum_instance;
}


inline AssumType infer_from_var(FunctionPtr f,
                                VertexAdaptor<op_var> var,
                                ClassPtr &out_class) {
  return calc_assumption_for_var(f, var->str_val, out_class);
}


inline AssumType infer_from_call(FunctionPtr f,
                                 VertexAdaptor<op_func_call> call,
                                 ClassPtr &out_class) {
  const std::string &fname = call->extra_type == op_ex_func_member
                             ? resolve_instance_func_name(f, call)
                             : get_full_static_member_name(f, call->str_val);

  const FunctionSetPtr &ptr = G->get_function_set(fs_function, fname, false);
  if (!ptr || !ptr->size()) {
    kphp_error(ptr && ptr->size() > 0, dl_pstr("%s() is undefined, can not infer class", fname.c_str()));
    return assum_unknown;
  }

  return calc_assumption_for_return(ptr[0], out_class);
}

inline AssumType infer_from_array(FunctionPtr f,
                                  VertexAdaptor<op_array> array,
                                  ClassPtr &out_class) {
  kphp_assert(array);
  for (auto v : *array) {
    ClassPtr inferred_class;
    AssumType assum = infer_class_of_expr(f, v, inferred_class);
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
                                   ClassPtr &out_class) {
  ClassPtr lhs_class;
  AssumType lhs_assum = infer_class_of_expr(f, prop->ith(0), lhs_class);

  if (lhs_assum != assum_instance) {
    return assum_not_instance;
  }

  return calc_assumption_for_class_var(lhs_class, prop->str_val, out_class);
}


/*
 * Главная функция, вызывающаяся извне: возвращает assumption для любого выражения root внутри f.
 */
AssumType infer_class_of_expr(FunctionPtr f, VertexPtr root, ClassPtr &out_class) {
  switch (root->type()) {
    case op_constructor_call:
      return infer_from_ctor(f, root, out_class);
    case op_var:
      return infer_from_var(f, root, out_class);
    case op_instance_prop:
      return infer_from_instance_prop(f, root, out_class);
    case op_func_call:
      return infer_from_call(f, root, out_class);
    case op_index:
      return root->size() == 2 && assum_instance_array == infer_class_of_expr(f, root->ith(0), out_class)
             ? assum_instance
             : assum_not_instance;
    case op_array:
      return infer_from_array(f, root, out_class);

    default:
      return assum_not_instance;
  }
}


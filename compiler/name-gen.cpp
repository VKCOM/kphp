#include "compiler/name-gen.h"

#include "common/algorithms/hashes.h"
#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/pipes/register-variables.h"
#include "compiler/stage.h"

string gen_anonymous_function_name(FunctionPtr function) {
  return gen_unique_name("anon", function);
}

bool is_anonymous_function_name(vk::string_view name) {
  return name.starts_with("anon$");
}

string gen_const_string_name(const string &str) {
  return fmt_format("const_string$us{:x}", vk::std_hash(str));
}

string gen_const_regexp_name(const string &str) {
  return fmt_format("const_regexp$us{:x}", vk::std_hash(str));
}

bool is_array_suitable_for_hashing(VertexPtr vertex) {
  return vertex->type() == op_array && CheckConst::is_const(vertex);
}

std::string gen_const_array_name(const VertexAdaptor<op_array> &array) {
  return fmt_format("const_array$us{:x}", ArrayHash::calc_hash(array));
}

string gen_unique_name(const string& prefix, FunctionPtr function) {
  if (!function) {
    function = stage::get_function();
  }
  kphp_assert(function);
  auto h = vk::std_hash(function->name);
  auto ph = vk::std_hash(prefix);
  auto &index = function->name_gen_map[ph];
  return replace_non_alphanum(prefix) + fmt_format("$u{:x}_{}", h, index++);
}

string resolve_uses(FunctionPtr current_function, string class_name, char delim) {
  if (current_function->class_id && current_function->class_id->is_trait()) {
    bool use_trait_inside_another_trait = current_function->type == FunctionData::func_class_holder;
    if (!use_trait_inside_another_trait) {
      return {};
    }
  }

  if (class_name[0] != '\\') {
    if (class_name == "parent") {
      // не parent_class->name, а именно поиск по str_dependents: resolve_uses() вызывается раньше, чем связка классов
      if (auto parent_class_name = current_function->get_this_or_topmost_if_lambda()->class_id->get_parent_class_name()) {
        class_name = *parent_class_name;
      } else {
        kphp_error_act(false, "Using parent:: in class that extends nothing", return {});
      }
    } else if (class_name == "static") {
      auto parent_function = current_function->get_this_or_topmost_if_lambda();
      class_name = parent_function->context_class->name;
    } else if (class_name == "self"){
      auto class_id = current_function->get_this_or_topmost_if_lambda()->class_id;
      kphp_error_act(class_id, "Can't resolve self, there is no any class context", return {});
      class_name = class_id->name;
    } else {
      size_t slash_pos = class_name.find('\\');
      if (slash_pos == string::npos) {
        slash_pos = class_name.length();
      }
      string class_name_start = class_name.substr(0, slash_pos);
      auto uses_it = current_function->file_id->namespace_uses.find(class_name_start);

      if (uses_it != current_function->file_id->namespace_uses.end()) {
        class_name = static_cast<std::string>(uses_it->second) + class_name.substr(class_name_start.length());
      } else {
        class_name = current_function->file_id->namespace_name + "\\" + class_name;
      }
    }
  }
  if (class_name[0] == '\\') {
    class_name = class_name.substr(1);
  }
  if (delim != '\\') {
    std::replace(class_name.begin(), class_name.end(), '\\', delim);
  }

  return class_name;
}

static std::string _err_instance_access(VertexPtr v, const std::string &desc) {
  if (v->type() == op_func_call) {
    return "Invalid call ...->" + v->get_string() + "(): " + desc;
  }

  return "Invalid property ...->" + v->get_string() + ": " + desc;
}

/*
 * Если 'new A(...)', то на самом деле это вызов A$$__construct(...), если не special case.
 */
string resolve_constructor_func_name(FunctionPtr function __attribute__ ((unused)), VertexAdaptor<op_constructor_call> ctor_call) {
  return replace_backslashes(ctor_call->get_string()) + "$$" + ClassData::NAME_OF_CONSTRUCT;
}

/*
 * На уровне gentree конструкция '...->method(...)' превращается в 'SOMEMETHOD(...,...)'.
 * Вот тут определяем, что за SOMEMETHOD — это из какого-то класса — именно того, что в левой части (= первый параметр).
 * Например, $a->method(), если $a имеет тип Classes\A, то на самом деле это Classes$A$$method
 */
string resolve_instance_func_name(FunctionPtr function, VertexAdaptor<op_func_call> arrow_call) {
  if (auto klass = resolve_class_of_arrow_access(function, arrow_call)) {
    vk::string_view name_of_func_called{arrow_call->get_string()};
    if (name_of_func_called.starts_with(replace_backslashes(klass->name))) {
      name_of_func_called = name_of_func_called.substr(klass->name.size() + strlen("$$"));
    }
    if (auto method = klass->get_instance_method(name_of_func_called)) {
      return method->function->name;
    }
  }

  return std::string();
}

ClassPtr resolve_class_of_arrow_access_helper(FunctionPtr function, VertexPtr v, VertexPtr lhs) {
  ClassPtr klass;
  switch (lhs->type()) {
    // (new A)->...
    case op_constructor_call: {
      return infer_class_of_expr(function, lhs).klass;
    }

    // $var->...
    case op_var: {
      Assumption a = infer_class_of_expr(function, lhs);
      kphp_error(a.assum_type == assum_instance,
                 _err_instance_access(v, fmt_format("${} is not an instance or it can't be detected\n"
                                                    "Add phpdoc @var to variable or @return to function was used to initialize it.",
                                                    lhs->get_string())));
      return a.klass;
    }

    // getInstance()->...
    case op_func_call: {
      Assumption a = infer_class_of_expr(function, lhs);
      kphp_error(a.assum_type == assum_instance,
                 _err_instance_access(v, fmt_format("{}() does not return instance or it can't be detected.\n"
                                                    "Add @return tag to function phpdoc",
                                                    lhs->get_string())));
      return a.klass;
    }

    // ...->anotherInstance->...
    case op_instance_prop: {
      Assumption a = infer_class_of_expr(function, lhs);
      kphp_error(a.assum_type == assum_instance,
                 _err_instance_access(v, fmt_format("${}->{} is not an instance or it can't be detected.\n"
                                                    "Add phpdoc @var to field declaration",
                                                    lhs.as<op_instance_prop>()->instance()->get_string(), lhs->get_string())));
      return a.klass;
    }

    // ...[$idx]->...
    case op_index: {
      VertexAdaptor<op_index> index = lhs.as<op_index>();
      VertexPtr array = index->array();
      if (index->has_key()) {
        // $var[$idx]->...
        if (array->type() == op_var) {
          Assumption a = infer_class_of_expr(function, array);
          kphp_error(a.assum_type == assum_instance_array,
                     _err_instance_access(v, fmt_format("${} is not an array of instances or it can't be detected.\n"
                                                        "Add phpdoc to variable or @return tag to function was used to initialize it.",
                                                        array->get_string())));
          return a.klass;
        }
        // getArr()[$idx]->...
        if (array->type() == op_func_call) {
          Assumption a = infer_class_of_expr(function, array);
          kphp_error(a.assum_type == assum_instance_array,
                     _err_instance_access(v, fmt_format("{}() does not return array of instances or it can't be detected.\n"
                                                        "Add @return tag to function phpdoc",
                                                        array->get_string())));
          return a.klass;
        }
        // ...->arrOfInstances[$idx]->...
        if (array->type() == op_instance_prop) {
          Assumption a = infer_class_of_expr(function, array);
          kphp_error(a.assum_type == assum_instance_array,
                     _err_instance_access(v, fmt_format("${}->{} is not array of instances or it can't be detected.\n"
                                                        "Add phpdoc to field declaration",
                                                        array.as<op_instance_prop>()->instance()->get_string(), array->get_string())));
          return a.klass;
        }
      }
      break;
    }

    // (clone $expr)->...
    case op_clone:
      return resolve_class_of_arrow_access_helper(function, v, lhs.as<op_clone>()->expr());

    // ({ $tmp = clone $expr; $tmp->clone(); $tmp })->...
    case op_seq_rval:
      return resolve_class_of_arrow_access_helper(function, v, lhs.as<op_seq_rval>()->back());

    default:
      break;
  }

  kphp_error(false, _err_instance_access(v, "Can not parse: maybe, too deep nesting"));
  return klass;
}

/*
 * Когда есть любое выражение lhs перед стрелочкой ('$a->...', '(new A())->...', 'get()->nestedArr[0]->...'),
 * то слева ожидается инстанс какого-то класса.
 * Определяем, что это за класс, и кидаем осмысленную ошибку, если там оказалось что-то не то.
 * Например, '$a = 42; $a->...' скажет, что '$a is not an instance'
 */
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr v) {
  // тут всего 2 варианта типа v:
  // 1) lhs->f(...args), что заменилось на f(lhs,...args)
  // 2) lhs->propname
  kphp_assert((v->type() == op_func_call && v->extra_type == op_ex_func_call_arrow) || v->type() == op_instance_prop);

  VertexPtr lhs = v->type() == op_instance_prop ?
                  v.as<op_instance_prop>()->instance() :
                  v.as<op_func_call>()->args()[0];

  return resolve_class_of_arrow_access_helper(function, v, lhs);
}


string get_context_by_prefix(FunctionPtr function, const string &class_name, char delim) {
  if (class_name == "static" || class_name == "self" || class_name == "parent") {
    return resolve_uses(function, "\\" + function->get_this_or_topmost_if_lambda()->context_class->name, delim);
  }
  return resolve_uses(function, class_name, delim);
}

string get_full_static_member_name(FunctionPtr function, const string &name, bool append_with_context) {
  size_t pos$$ = name.find("::");

  if (pos$$ == string::npos) {
    return name;
  }

  string prefix_name = name.substr(0, pos$$);
  string class_name = resolve_uses(function, prefix_name, '$');
  if (class_name.empty()) {
    return {};
  }

  string fun_name = name.substr(pos$$ + 2);
  string new_name = class_name + "$$" + fun_name;
  if (!function->is_lambda() && append_with_context) {
    string context_name = get_context_by_prefix(function, prefix_name);
    if (context_name != class_name) {
      new_name += "$$" + context_name;
    }
  }
  return new_name;
}

string resolve_define_name(string name) {
  size_t pos$$ = name.find("$$");
  if (pos$$ != string::npos) {
    string class_name = name.substr(0, pos$$);
    string define_name = name.substr(pos$$ + 2);
    const string &real_class_name = replace_characters(class_name, '$', '\\');
    ClassPtr klass = G->get_class(real_class_name);
    if (klass) {
      while (klass && !klass->members.has_constant(define_name)) {
        klass = klass->get_parent_or_interface();
      }
      if (klass) {
        name = "c#" + replace_backslashes(klass->name) + "$$" + define_name;
      }
    }
  }
  return name;
}

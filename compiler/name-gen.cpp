// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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

string resolve_uses(FunctionPtr resolve_context, string class_name, char delim) {
  if (class_name[0] != '\\') {
    if (class_name == "parent") {
      // do a str_dependents search instead of using parent_class->name:
      // resolve_uses() is called before the classes are resolved
      if (auto parent_class_name = resolve_context->get_this_or_topmost_if_lambda()->class_id->get_parent_class_name()) {
        class_name = *parent_class_name;
      } else {
        kphp_error_act(resolve_context->class_id && resolve_context->class_id->is_trait(),
                       "Using parent:: in class that extends nothing", return {});
      }
    } else if (class_name == "static") {
      auto parent_function = resolve_context->get_this_or_topmost_if_lambda();
      class_name = parent_function->context_class->name;
    } else if (class_name == "self") {
      auto class_id = resolve_context->get_this_or_topmost_if_lambda()->class_id;
      kphp_error_act(class_id, "Can't resolve self, there is no any class context", return {});
      class_name = class_id->name;
    } else {
      size_t slash_pos = class_name.find('\\');
      if (slash_pos == string::npos) {
        slash_pos = class_name.length();
      }
      string class_name_start = class_name.substr(0, slash_pos);
      auto uses_it = resolve_context->file_id->namespace_uses.find(class_name_start);

      if (uses_it != resolve_context->file_id->namespace_uses.end()) {
        class_name = static_cast<std::string>(uses_it->second) + class_name.substr(class_name_start.length());
      } else {
        class_name = resolve_context->file_id->namespace_name + "\\" + class_name;
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
 * gentree turns 'x->method(...)' into 'SOMEMETHOD(x, ...)'.
 * We deduce that SOMEMETHOD belongs to the class type of the first argument (x in the example above).
 * For example, $a->method, if $a is Classes\A, then Classes$A$$method is returned.
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
  switch (lhs->type()) {
    case op_alloc:
      return lhs.as<op_alloc>()->allocated_class;
    // $var->...
    case op_var: {
      auto klass = infer_class_of_expr(function, lhs).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("${} is not an instance or it can't be detected\n"
                                                    "Add phpdoc @var to variable or @return to function was used to initialize it.",
                                                    lhs->get_string())));
      return klass;
    }

    // getInstance()->...
    case op_func_call: {
      auto klass = infer_class_of_expr(function, lhs).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("{}() does not return instance or it can't be detected.\n"
                                                    "Add @return tag to function phpdoc",
                                                    lhs->get_string())));
      return klass;
    }

    // ...->anotherInstance->...
    case op_instance_prop: {
      auto klass = infer_class_of_expr(function, lhs).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("${}->{} is not an instance or it can't be detected.\n"
                                                    "Add phpdoc @var to field declaration",
                                                    lhs.as<op_instance_prop>()->instance()->get_string(), lhs->get_string())));
      return klass;
    }

    // ...[$idx]->...
    case op_index: {
      VertexAdaptor<op_index> index = lhs.as<op_index>();
      VertexPtr array = index->array();
      if (index->has_key()) {
        // $var[$idx]->...
        if (array->type() == op_var) {
          auto as_inner_klass = infer_class_of_expr(function, array).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("${} is not an array of instances or it can't be detected.\n"
                                                        "Add phpdoc to variable or @return tag to function was used to initialize it.",
                                                        array->get_string())));
          return as_inner_klass;
        }
        // getArr()[$idx]->...
        if (array->type() == op_func_call) {
          auto as_inner_klass = infer_class_of_expr(function, array).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("{}() does not return array of instances or it can't be detected.\n"
                                                        "Add @return tag to function phpdoc",
                                                        array->get_string())));
          return as_inner_klass;
        }
        // ...->arrOfInstances[$idx]->...
        if (array->type() == op_instance_prop) {
          auto as_inner_klass = infer_class_of_expr(function, array).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("${}->{} is not array of instances or it can't be detected.\n"
                                                        "Add phpdoc to field declaration",
                                                        array.as<op_instance_prop>()->instance()->get_string(), array->get_string())));
          return as_inner_klass;
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
  return ClassPtr{};
}

/*
 * For every lhs before the arrow ('$a->...', '(new A())->...', 'get()->nestedArr[0]->...'),
 * we expect some class instance on the left side.
 * Deduce that class and throw a meaningful error if something went wrong.
 * For example, the code '$a = 42; $a->...' will result in '$a is not an instance' error.
 */
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr v) {
  // there are only 2 possible forms of v:
  // 1) lhs->f(...args), that was replaced with f(lhs,...args)
  // 2) lhs->propname
  kphp_assert((v->type() == op_func_call && v->extra_type == op_ex_func_call_arrow) || v->type() == op_instance_prop);

  VertexPtr lhs = v->type() == op_instance_prop ?
                  v.as<op_instance_prop>()->instance() :
                  v.as<op_func_call>()->args()[0];

  return resolve_class_of_arrow_access_helper(function, v, lhs);
}


string get_context_by_prefix(FunctionPtr function, const string &class_name, char delim) {
  if (is_string_self_static_parent(class_name)) {
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
    if (auto klass = G->get_class(real_class_name)) {
      ClassPtr last_ancestor;
      for (const auto &ancestor : klass->get_all_ancestors()) {
        if (ancestor->members.has_constant(define_name)) {
          if (!last_ancestor) {
            name = "c#" + replace_backslashes(ancestor->name) + "$$" + define_name;
            last_ancestor = ancestor;
          } else if (!ancestor->is_parent_of(last_ancestor)) {
            return {};
          }
        }
      }
    }
  }
  return name;
}

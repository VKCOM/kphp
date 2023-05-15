// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/name-gen.h"

#include "common/algorithms/hashes.h"
#include "common/wrappers/likely.h"

#include "compiler/type-hint.h"
#include "compiler/compiler-core.h"
#include "compiler/const-manipulations.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/pipes/register-variables.h"
#include "compiler/stage.h"

std::string gen_anonymous_scope_name(FunctionPtr function) {
  return gen_unique_name("cdef", function);
}

std::string gen_anonymous_function_name(FunctionPtr function) {
  return gen_unique_name("lambda", function);
}

std::string gen_const_string_name(const std::string &str) {
  return fmt_format("const_string$us{:x}", vk::std_hash(str));
}

std::string gen_const_regexp_name(const std::string &str) {
  return fmt_format("const_regexp$us{:x}", vk::std_hash(str));
}

bool is_array_suitable_for_hashing(VertexPtr vertex) {
  return vertex->type() == op_array && CheckConst::is_const(vertex);
}

// checks that inlined as define' value constructor is suitable to be stored as constant var
bool is_object_suitable_for_hashing(VertexPtr vertex) {
  return vertex->type() == op_define_val && vertex.as<op_define_val>()->value()->type() == op_func_call
         && vertex.as<op_define_val>()->value()->extra_type == op_ex_constructor_call && vertex->const_type == cnst_const_val;
}

std::string gen_const_object_name(const VertexAdaptor<op_define_val> &def) {
  kphp_assert_msg(def->value()->type() == op_func_call, "Internal error: expected op_define_val <op_func_call>");
  auto obj_hash = ObjectHash::calc_hash(def);
  return fmt_format("const_obj$us{:x}", obj_hash);
}

std::string gen_const_array_name(const VertexAdaptor<op_array> &array) {
  return fmt_format("const_array$us{:x}", ArrayHash::calc_hash(array));
}

std::string gen_unique_name(const std::string& prefix, FunctionPtr function) {
  if (!function) {
    function = stage::get_function();
  }
  kphp_assert(function);
  auto h = vk::std_hash(function->name);
  auto ph = vk::std_hash(prefix);
  auto &index = function->name_gen_map[ph];
  return fmt_format("{}$u{:x}_{}", prefix, h, index++);
}

std::string resolve_uses(FunctionPtr resolve_context, const std::string &class_name) {
  if (class_name[0] == '\\') {
    return class_name.substr(1);
  }

  if (class_name == "parent") {
    // do a str_dependents search instead of using parent_class->name:
    // resolve_uses() is called before the classes are resolved
    if (const auto *parent_name = resolve_context->get_this_or_topmost_if_lambda()->class_id->get_parent_class_name()) {
      return *parent_name;
    }
    kphp_error(resolve_context->class_id && resolve_context->class_id->is_trait(),
               "Using parent:: in class that extends nothing");
    return {};
  }

  if (class_name == "static") {
    ClassPtr context_class = resolve_context->get_this_or_topmost_if_lambda()->context_class;
    kphp_error_act(context_class, "Can't resolve static, there is no any class context", return {});
    return context_class->name;
  }

  if (class_name == "self") {
    ClassPtr class_id = resolve_context->get_this_or_topmost_if_lambda()->class_id;
    kphp_error_act(class_id, "Can't resolve self, there is no any class context", return {});
    return class_id->name;
  }

  size_t slash_pos = class_name.find('\\');
  if (slash_pos == std::string::npos) {
    slash_pos = class_name.length();
  }

  auto uses_it = resolve_context->file_id->namespace_uses.find(class_name.substr(0, slash_pos));
  if (uses_it != resolve_context->file_id->namespace_uses.end()) {
    return static_cast<std::string>(uses_it->second) + class_name.substr(slash_pos);
  }
  if (!resolve_context->file_id->namespace_name.empty()) {
    return resolve_context->file_id->namespace_name + "\\" + class_name;
  }

  return class_name;
}

static std::string __attribute__((noinline)) _safe_str(VertexPtr v) {
  return v->has_get_string() ? v->get_string() : "...";
}

static std::string __attribute__((noinline)) _err_instance_access(VertexPtr v, const std::string &desc) {
  if (v->type() == op_func_call) {
    return "Invalid call ...->" + v->get_string() + "(): " + desc;
  }

  return "Invalid property ...->" + v->get_string() + ": " + desc;
}

/*
 * For every lhs before the arrow ('$a->...', '(new A())->...', 'get()->nestedArr[0]->...'),
 * we expect some class instance on the left side.
 * Deduce that class and throw a meaningful error if something went wrong.
 * For example, the code '$a = 42; $a->...' will result in '$a is not an instance' error.
 */
ClassPtr resolve_class_of_arrow_access(FunctionPtr function, VertexPtr lhs, VertexPtr v) {
  switch (lhs->type()) {
    // $var->...
    case op_var: {
      auto klass = assume_class_of_expr(function, lhs, v).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v,
                                      function->find_param_by_name(lhs->get_string())
                                      ? fmt_format("${} is not an instance or it can't be detected.\n"
                                                   "Add /** @param {} ${} */ to phpdoc above {}.",
                                                   _safe_str(lhs), "{type}", _safe_str(lhs), function->as_human_readable())
                                      : fmt_format("${} is not an instance or it can't be detected.\n"
                                                   "Add /** @var {} ${} */ above the first assignment to ${}.",
                                                   _safe_str(lhs), "{type}", _safe_str(lhs), _safe_str(lhs))
                 ));
      return klass;
    }

    // getInstance()->...
    case op_func_call: {
      auto klass = assume_class_of_expr(function, lhs, v).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("{}() does not return an instance or it can't be detected.\n"
                                                    "Add @return tag to function phpdoc.",
                                                    _safe_str(lhs))));
      return klass;
    }

    // $callback()->...
    case op_invoke_call: {
      auto klass = assume_class_of_expr(function, lhs, v).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("A callback does not return an instance or it can't be detected.\n"
                                                    "Extract the result of a callback invocation to a separate variable, providing @var phpdoc.")));
      return klass;
    }

    // ...->anotherInstance->...
    case op_instance_prop: {
      auto klass = assume_class_of_expr(function, lhs, v).try_as_class();
      kphp_error(klass,
                 _err_instance_access(v, fmt_format("{}->{} is not an instance or it can't be detected.\n"
                                                    "Provide /** @var {} */ above the field declaration or use a field type hint.",
                                                    _safe_str(lhs.as<op_instance_prop>()->instance()), _safe_str(lhs), "{type}")));
      return klass;
    }

    // ...[$idx]->...
    case op_index: {
      VertexAdaptor<op_index> index = lhs.as<op_index>();
      VertexPtr array = index->array();
      if (index->has_key()) {
        // $var[$idx]->...
        if (array->type() == op_var) {
          auto as_inner_klass = assume_class_of_expr(function, array, v).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("${} is not an array of instances or it can't be detected.\n"
                                                        "Add /** @var {}[] ${} */ above the first assignment to ${}.",
                                                        _safe_str(array), "{type}", _safe_str(array), _safe_str(array))));
          return as_inner_klass;
        }
        // getArr()[$idx]->...
        if (array->type() == op_func_call) {
          auto as_inner_klass = assume_class_of_expr(function, array, v).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("{}() does not return an array of instances or it can't be detected.\n"
                                                        "Add @return tag to function phpdoc.",
                                                        _safe_str(array))));
          return as_inner_klass;
        }
        // ...->arrOfInstances[$idx]->...
        if (array->type() == op_instance_prop) {
          auto as_inner_klass = assume_class_of_expr(function, array, v).get_subkey_by_index(index->key()).try_as_class();
          kphp_error(as_inner_klass,
                     _err_instance_access(v, fmt_format("{}->{} is not array of instances or it can't be detected.\n"
                                                        "Provide /** @var {}[] */ above the field declaration.",
                                                        _safe_str(array.as<op_instance_prop>()->instance()), _safe_str(array), "{type}")));
          return as_inner_klass;
        }
      }
      break;
    }

    // (clone $expr)->...
    case op_clone:
      return resolve_class_of_arrow_access(function, lhs.as<op_clone>()->expr(), v);

    // ({ $tmp = clone $expr; $tmp->clone(); $tmp })->...
    case op_seq_rval:
      return resolve_class_of_arrow_access(function, lhs.as<op_seq_rval>()->back(), v);

    // ($a = new A)->...
    case op_set:
      return resolve_class_of_arrow_access(function, lhs.as<op_set>()->rhs(), v);

    // inlined constant value
    case op_define_val:
      return resolve_class_of_arrow_access(function, lhs.as<op_define_val>()->value(), v);

    default:
      break;
  }

  kphp_error(false, _err_instance_access(v, "Can not parse: maybe, too deep nesting"));
  return ClassPtr{};
}

std::string resolve_static_method_append_context(FunctionPtr f, const std::string &prefix_name, const std::string &class_name, const std::string &local_name) {
  if (is_string_self_static_parent(prefix_name)) {
    const std::string &context_name = f->get_this_or_topmost_if_lambda()->context_class->name;
    if (context_name != class_name) {
      return replace_characters(class_name + "$$" + local_name + "$$" + context_name, '\\', '$');
    }
  }

  return replace_characters(class_name, '\\', '$') + "$$" + local_name;
}

std::string resolve_define_name(std::string name) {
  size_t pos$$ = name.find("$$");
  if (pos$$ != std::string::npos) {
    std::string class_name = name.substr(0, pos$$);
    std::string define_name = name.substr(pos$$ + 2);
    const std::string &real_class_name = replace_characters(class_name, '$', '\\');
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

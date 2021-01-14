// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

/*
 * 'Assumption' is predicted type information that describes which variables belong to which classes.
 * Used to resolve the arrow method calls and find which actual function is being called.
 * For $a->getValue() we can infer that getValue() belongs to the A class; hence we resolve it to Classes$A$$getValue().
 *
 * Methods resolving happens in "Preprocess functions C pass" (try_set_func_id).
 * That pass happens before the type inferring and register variables, this is why assumptions relies on variable names.
 * Parsing of @return/@param/@type/@var for variables and class members is implemented here.
 *
 * We also perform a simple function code analysis that helps infer some types without explicit phpdoc hints.
 * For example, in "$a = new A; $a->foo()" we see that A constructor is called, therefore the type of $a is A
 * and $a->foo can be resolved to Classes$A$$foo().
 *
 * Caution! Assumptions is NOT type inferring, it's much more rough analysis. They are used only to bind -> invocations.
 * They do not contain information about primitives, etc., they do not operate cfg/smartcasts — only variable names.
 * In other words, assumptions are our type expectations/intentions/predictions, they are checked by tinf later.
 * If they are incorrect (for example, @return A but actually returns B), a tinf error will occur later on.
 */
#include "compiler/class-assumptions.h"

#include <thread>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
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

bool assumption_merge(vk::intrusive_ptr<Assumption> &dst, const vk::intrusive_ptr<Assumption> &rhs) {
  if (dst == rhs) return true;

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
    for (auto &it : dst_as_shape->subkeys_assumptions) {
      auto at_it = rhs_as_shape->subkeys_assumptions.find(it.first);
      if (at_it != rhs_as_shape->subkeys_assumptions.end()) {
        ok &= assumption_merge(it.second, at_it->second);
      }
    }
    return ok;
  }

  auto dst_as_typed_callable = dst.try_as<AssumTypedCallable>();
  auto rhs_as_typed_callable = rhs.try_as<AssumTypedCallable>();
  if (dst_as_typed_callable && rhs_as_typed_callable) {
    return dst_as_typed_callable->interface == rhs_as_typed_callable->interface;
  }

  if (dst_as_typed_callable && rhs_as_instance) {
    return rhs_as_instance->klass->is_lambda();
  }
  if (rhs_as_typed_callable && dst_as_instance && dst_as_instance->klass->is_lambda()) {
    dst = rhs_as_typed_callable;
    return true;
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
    return;
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
 * After we parsed the @param/@return/@var tag contents as TypeHint, try to find a class there.
 */
vk::intrusive_ptr<Assumption> assumption_create_from_phpdoc(const TypeHint *type_hint) {
  // 'A|null' => 'A'
  // 'A[]|false' => 'A[]'
  // 'null|some_class' => 'some_class'
  // 'mixed|mixed[]' => primitive
  if (const auto *as_optional = type_hint->try_as<TypeHintOptional>()) {
    type_hint = as_optional->inner;
  }
  if (const auto *as_pipe = type_hint->try_as<TypeHintPipe>()) {
    for (const auto *item : as_pipe->items) {
      if (!item->try_as<TypeHintPrimitive>()) {
        type_hint = item;
      }
    }
  }

  if (const auto *as_instance = type_hint->try_as<TypeHintInstance>()) {
    ClassPtr instance = as_instance->resolve();
    kphp_error(instance, fmt_format("Can't find class {}", as_instance->full_class_name));
    return instance ? AssumInstance::create(instance) : AssumNotInstance::create();
  }
  if (const auto *as_array = type_hint->try_as<TypeHintArray>()) {
    auto sub = assumption_create_from_phpdoc(as_array->inner);
    return AssumArray::create(assumption_create_from_phpdoc(as_array->inner));
  }
  if (const auto *as_tuple = type_hint->try_as<TypeHintTuple>()) {
    decltype(AssumTuple::subkeys_assumptions) sub;
    for (const TypeHint *item : as_tuple->items) {
      sub.emplace_back(assumption_create_from_phpdoc(item));
    }
    return AssumTuple::create(std::move(sub));
  }
  if (const auto *as_shape = type_hint->try_as<TypeHintShape>()) {
    decltype(AssumShape::subkeys_assumptions) sub;
    for (const auto &item : as_shape->items) {
      sub.emplace(item.first, assumption_create_from_phpdoc(item.second));
    }
    return AssumShape::create(std::move(sub));
  }
  if (const auto *as_callable = type_hint->try_as<TypeHintCallable>()) {
    // typed callable, like callable(int):void — when we see such a phpdoc, we generate an interface based on types
    // (they don't turn a function into templates: $cb(1) => $cb->__invoke(1) => $cb is still AssumTypedCallable inside f)
    if (as_callable->is_typed_callable() && !as_callable->has_argref_inside()) {
      return AssumTypedCallable::create(as_callable->get_interface());
    }
    // just `callable` keywords have sense only as a param type hint
    // they turn the function into a template one, and each instantiation will have instances as a parameter assumption
  }

  return AssumNotInstance::create();
}

/*
 * If a class property is an array/other class instance, it should be marked with this phpdoc:
 * / ** @var A * / var $aInstance;
 * We recognize such phpdocs and var defs inside classes.
 */
void analyze_phpdoc_of_class_field(ClassPtr c, vk::string_view var_name, vk::string_view phpdoc_str) {
  if (auto parsed = phpdoc_find_tag(phpdoc_str, php_doc_tag::var, c->get_holder_function())) {
    if (parsed.var_name.empty() || var_name == parsed.var_name) {
      // todo rewrite in later, try not to parse phpdoc at all
      auto resolved_type_hint = parsed.type_hint->has_self_static_parent_inside() ? parsed.type_hint->replace_self_static_parent(c->get_holder_function()) : parsed.type_hint;
      assumption_add_for_var(c, var_name, assumption_create_from_phpdoc(resolved_type_hint));
    }
  }
}


/*
 * Deduce which type is assigned to $a in '$a = ...rhs...'
 */
void analyze_set_to_var(FunctionPtr f, vk::string_view var_name, const VertexPtr &rhs, size_t depth) {
  auto a = infer_class_of_expr(f, rhs, depth + 1);

  if (!a->is_primitive()) {
    assumption_add_for_var(f, var_name, a);
  }
}

/*
 * Deduce which type is assigned to $lhs_var in 'list(... $lhs_var ...) = ...rhs...';
 * (rhs may contain shape or tuple)
 */
void analyze_set_to_list_var(FunctionPtr f, vk::string_view var_name, VertexPtr index_key, const VertexPtr &rhs, size_t depth) {
  auto a = infer_class_of_expr(f, rhs, depth + 1);

  if (!a->get_subkey_by_index(index_key)->is_primitive()) {
    assumption_add_for_var(f, var_name, a->get_subkey_by_index(index_key));
  }
}

/*
 * Deduce that $ex is T-typed from `catch (T $ex)`
 */
void analyze_catch_of_var(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_catch> root) {
  kphp_assert(root->exception_class);
  assumption_add_for_var(f, var_name, AssumInstance::create(root->exception_class));
}

/*
 * Deduce that $a is an instance of T if $arr is T[] in 'foreach($arr as $a)'
 */
static void analyze_foreach(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_foreach_param> root, size_t depth) {
  auto a = infer_class_of_expr(f, root->xs(), depth + 1);

  if (auto as_array = a.try_as<AssumArray>()) {
    assumption_add_for_var(f, var_name, as_array->inner);
  }
}

/*
 * Deduce that $MC is Memcache from 'global $MC'.
 * Variables like $MC, $MC_Local and other are builtins with a known type.
 */
void analyze_global_var(FunctionPtr f, vk::string_view var_name) {
  if (vk::any_of_equal(var_name, "MC", "MC_Local", "MC_Ads", "MC_Log", "PMC", "mc_fast", "MC_Config", "MC_Stats")) {
    assumption_add_for_var(f, var_name, AssumInstance::create(G->get_memcache_class()));
  }
}


/*
 * For the '$a->...' expression (where $a is a local variable) we need to deduce type of $a.
 * This function tries to accomplish that.
 */
void calc_assumptions_for_var_internal(FunctionPtr f, vk::string_view var_name, VertexPtr root, size_t depth) {
  switch (root->type()) {
    // assumptions pass happens early in the pipeline, so /** phpdocs with @var */ are not turned into the op_phpdoc_var yet
    case op_phpdoc_raw: {
      for (const auto &parsed : phpdoc_find_tag_multi(root.as<op_phpdoc_raw>()->phpdoc_str, php_doc_tag::var, f)) {
        // both '/** @var A $v */' and '/** @var A */ $v ...' forms are supported
        const std::string &var_name = parsed.var_name.empty() ? root.as<op_phpdoc_raw>()->next_var_name : parsed.var_name;
        if (!var_name.empty()) {
          // @var phpdocs can contain 'self' and others, that will be resolved only after registering variables
          auto resolved_type_hint = parsed.type_hint->has_self_static_parent_inside() ? parsed.type_hint->replace_self_static_parent(f) : parsed.type_hint;
          assumption_add_for_var(f, var_name, assumption_create_from_phpdoc(resolved_type_hint));
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

      for (auto x : list->list()) {
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
      for (auto c : t->catch_list()) {
        auto catch_op = c.as<op_catch>();
        if (catch_op->var()->type() == op_var && catch_op->var()->get_string() == var_name) {
          analyze_catch_of_var(f, var_name, catch_op);
        }
      }
      break;    // break instead of return so we enter the 'try' block
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
 * For the '$a->...' expression (where $a is a function argument) we need to deduce argument types.
 * That type information comes in two forms: a @param phpdoc or a type hint like in 'f(A $a)'.
 * Called exactly once for every function.
 */
void init_assumptions_for_arguments(FunctionPtr f) {
  if (!f->phpdoc_str.empty()) {
    for (const auto &parsed : phpdoc_find_tag_multi(f->phpdoc_str, php_doc_tag::param, f)) {
      if (!parsed.var_name.empty()) {
        assumption_add_for_var(f, parsed.var_name, assumption_create_from_phpdoc(parsed.type_hint));
      }
    }
  }

  for (auto i : f->get_params()) {
    auto param = i.as<op_func_param>();
    if (param && param->type_hint) {
      auto a = assumption_create_from_phpdoc(param->type_hint);
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

  // using as_string and do a an adhoc parsing as "T $arg" can't be turned into a type hint (class T does not exist yet)
  for (const auto &kphp_template_str : phpdoc_find_tag_as_string_multi(f->phpdoc_str, php_doc_tag::kphp_template)) {
    // @kphp-template $arg is skipped, we're looking for
    // @kphp-template T $arg (as T is used to express the @kphp-return)
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
      auto a = calc_assumption_for_argument(f, template_arg_name);

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
 * For the '$a = getSome(), $a->... , or getSome()->...' we need to deduce the result type of getSome().
 * That information can be obtained from the @return tag or trivial returns analysis inside that function.
 * Called exactly once for every function.
 */
void init_assumptions_for_return(FunctionPtr f, VertexAdaptor<op_function> root) {
  assert (f->assumption_return_status == AssumptionStatus::processing);
//  printf("[%d] init_assumptions_for_return of %s\n", get_thread_id(), f->name.c_str());

  if (!f->phpdoc_str.empty()) {
    if (auto parsed = phpdoc_find_tag(f->phpdoc_str, php_doc_tag::returns, f)) {
      // todo rewrite it later, try not to parse phpdoc at all
      auto resolved_type_hint = parsed.type_hint->has_self_static_parent_inside() ? parsed.type_hint->replace_self_static_parent(f) : parsed.type_hint;
      assumption_add_for_return(f, assumption_create_from_phpdoc(resolved_type_hint));
      return;
    }

    if (parse_kphp_return_doc(f)) {
      return;
    }
  }
  if (f->return_typehint && !f->return_typehint->try_as<TypeHintCallable>()) {
    assumption_add_for_return(f, assumption_create_from_phpdoc(f->return_typehint));
    return;
  }

  // traverse function body, find all 'return' there
  // note, that for strong typed code, with everything annotated with @return, it isn't executed: returned earlier
  std::function<void(VertexPtr)> search_for_returns = [&search_for_returns, f](VertexPtr i) {
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

    for (auto child : *i) {
      search_for_returns(child);
    }
  };
  search_for_returns(root->cmd());

  if (!f->assumption_for_return) {
    assumption_add_for_return(f, AssumNotInstance::create());
  }
}

/*
 * If we know that $a is A and we have an expression like '$a->chat->...' we need to deduce the chat type.
 * That information can be obtained from the @var tag near the 'public $chat' declaration.
 * Called exactly once for every class.
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
    init_assumptions_for_arguments(f);
    f->assumption_args_status = AssumptionStatus::initialized;
  } else if (expected == AssumptionStatus::processing) {
    assumption_busy_wait(f->assumption_args_status);
  }

  return assumption_get_for_var(f, var_name);
}

/*
 * A high-level function which deduces the type of $a inside f.
 * The results are cached; init on f is called during the first invocation.
 * Always returns a non-null assumption.
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
  if (pos != std::string::npos) {   // static class variable that is used inside a function
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
 * A high-level function which deduces the result type of f.
 * The results are cached; init on f is called during the first invocation.
 * Always returns a non-null assumption.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call) {
  if (f->is_extern()) {
    if (f->return_typehint) {
      if (const auto *as_instance = f->return_typehint->try_as<TypeHintInstance>()) {
        return AssumInstance::create(as_instance->resolve());
      }
      if (const auto *as_arg_ref = f->return_typehint->try_as<TypeHintArgRefInstance>()) {
        if (auto arg = GenTree::get_call_arg_ref(as_arg_ref->arg_num, call)) {
          if (auto *class_name = GenTree::get_constexpr_string(arg)) {
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
      // we're checking for recursion inside a single thread, so the atomic operations order doesn't matter much;
      // if another thread modifies assumption_return_status but fails to set assumption_return_processing_thread in time,
      // we still won't enter this branch as assumption_return_processing_thread will have a default value
      return AssumNotInstance::create();
    }
    assumption_busy_wait(f->assumption_return_status);
  }

  return f->assumption_for_return;
}

/*
 * A high-level function which deduces the type of ->nested inside $a instance of the class c.
 * The results are cached; calls the class init once.
 * Always returns a non-null assumption.
 */
vk::intrusive_ptr<Assumption> calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name) {
  auto expected = AssumptionStatus::unknown;
  if (c->assumption_vars_status.compare_exchange_strong(expected, AssumptionStatus::processing)) {
    init_assumptions_for_all_fields(c);   // both instance and static variables
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

  // for builtin functions like array_pop/array_filter/etc with array of instances
  if (ptr->return_typehint && ptr->return_typehint->has_argref_inside()) {
    if (const auto *as_arg_ref = ptr->return_typehint->try_as<TypeHintArgRef>()) {       // array_values ($a ::: array) ::: ^1
      return infer_class_of_expr(f, GenTree::get_call_arg_ref(as_arg_ref->arg_num, call), depth + 1);
    }
    if (const auto *as_sub = ptr->return_typehint->try_as<TypeHintArgSubkeyGet>()) {  // array_shift (&$a ::: array) ::: ^1[*]
      if (const auto *arg_ref = as_sub->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref->arg_num, call), depth + 1);
        if (auto arr_a = expr_a.try_as<AssumArray>()) {
          return arr_a->inner;
        }
      }
      return AssumNotInstance::create();
    }
    if (const auto *as_array = ptr->return_typehint->try_as<TypeHintArray>()) {  // f (...) ::: array <^2>
      if (const auto *as_arg_ref = as_array->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(as_arg_ref->arg_num, call), depth + 1);
        if (auto inst_a = expr_a.try_as<AssumTypedCallable>()) {
          return AssumArray::create(inst_a->interface);
        }
        return AssumNotInstance::create();
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
  // this code handles '$x = $y ?: $z' operation;
  // it's converted to '$x = bool($tmp_var = $y) ? move($tmp_var) : $z'
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
 * Main functions that are called from the outside: returns the assumptions for any root expression inside f.
 * Never returns null (AssumNotInstance is used as a sentinel value).
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
    case op_exception_constructor_call:
      return infer_from_call(f, root.as<op_exception_constructor_call>()->constructor_call(), depth + 1);
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
  return "primitive";
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

std::string AssumTypedCallable::as_human_readable() const {
  return interface->name + "::_invoke";
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

bool AssumTypedCallable::is_primitive() const {
  return false;
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

vk::intrusive_ptr<Assumption> AssumTypedCallable::get_subkey_by_index(VertexPtr index_key __attribute__ ((unused))) const {
  return AssumNotInstance::create();
}


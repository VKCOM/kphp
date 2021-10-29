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
 *
 * We also perform a simple function code analysis that helps infer some types without explicit phpdoc hints.
 * For example, in "$a = new A; $a->foo()" we see that A constructor is called, therefore the type of $a is A
 * and $a->foo can be resolved to Classes$A$$foo().
 *
 * Caution! Assumptions is NOT type inferring, it's much more rough analysis. They are used ONLY to bind -> invocations.
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


Assumption::Assumption(const TypeHint *type_hint) {
  if (type_hint == nullptr) {
    // an undefined state
    assum_hint = nullptr;
  } else if (!type_hint->has_instances_inside() && !type_hint->has_callables_inside()) {
    // a not instance state
    assum_hint = TypeHintPrimitive::create(tp_any);

  } else if (const auto *as_optional = type_hint->try_as<TypeHintOptional>()) {
    assum_hint = as_optional->inner;    // if we have ?A, save A as an assumption
  } else {
    assum_hint = type_hint; // note, that tuple(int,A) is saved as is, but getting [0] subkey will return not instance
  }
}

Assumption::Assumption(ClassPtr klass) : assum_hint(TypeHintInstance::create(klass->name)) {
}

std::string Assumption::as_human_readable() const {
  return is_undefined() ? "undefined" : !has_instance() ? "not instance" : assum_hint->as_human_readable();
}

bool Assumption::has_instance() const {
  return assum_hint != nullptr && assum_hint->try_as<TypeHintPrimitive>() == nullptr;
}

Assumption Assumption::get_inner_if_array() const {
  if (assum_hint == nullptr) {
    return Assumption::undefined();
  }
  if (const auto *as_array = assum_hint->try_as<TypeHintArray>()) {
    return Assumption(as_array->inner);
  }
  return Assumption::undefined();
}

Assumption Assumption::get_subkey_by_index(VertexPtr index_key) const {
  if (assum_hint == nullptr) {
    return Assumption::undefined();
  }
  if (const auto *as_array = assum_hint->try_as<TypeHintArray>()) {
    return Assumption(as_array->inner);
  }
  if (const auto *as_tuple = assum_hint->try_as<TypeHintTuple>()) {
    if (auto as_int_index = GenTree::get_actual_value(index_key).try_as<op_int_const>()) {
      int int_index = parse_int_from_string(as_int_index);
      if (int_index >= 0 && int_index < as_tuple->items.size()) {
        return Assumption(as_tuple->items[int_index]);
      }
    }
  }
  if (const auto *as_shape = assum_hint->try_as<TypeHintShape>()) {
    if (vk::any_of_equal(GenTree::get_actual_value(index_key)->type(), op_int_const, op_string)) {
      const auto &string_index = GenTree::get_actual_value(index_key)->get_string();
      if (const TypeHint *at_index = as_shape->find_at(string_index)) {
        return Assumption(at_index);
      }
    }
  }
  return Assumption::undefined();
}

Assumption Assumption::not_instance() {
  return Assumption(TypeHintPrimitive::create(tp_any));
}

Assumption Assumption::undefined() {
  return {};
}

static ClassPtr extract_instance_from_ffi_type(const std::string &scope_name, const FFIType *type) {
  if (const auto *builtin = ffi_builtin_type(type->kind)) {
    return G->get_class(builtin->php_class_name);
  }
  if (vk::any_of_equal(type->kind, FFITypeKind::Struct, FFITypeKind::StructDef, FFITypeKind::Union)) {
    return G->get_class(FFIRoot::cdata_class_name(scope_name, type->str));
  }
  if (type->kind == FFITypeKind::Pointer) {
    return extract_instance_from_ffi_type(scope_name, type->members[0]);
  }
  return {};
}

ClassPtr Assumption::extract_instance_from_type_hint(const TypeHint *a) {
  if (a == nullptr) {
    return ClassPtr{};
  }
  if (const auto *as_optional = a->try_as<TypeHintOptional>()) {
    a = as_optional->inner;
  }

  if (const auto *as_ffi = a->try_as<TypeHintFFIType>()) {
    return extract_instance_from_ffi_type(as_ffi->scope_name, as_ffi->type);
  }
  if (const auto *as_ffi_scope = a->try_as<TypeHintFFIScope>()) {
    return G->get_class(FFIRoot::scope_class_name(as_ffi_scope->scope_name));
  }

  if (const auto *as_pipe = a->try_as<TypeHintPipe>()) {
    kphp_assert(as_pipe->items.size() >= 2);
    // to avoid the redundant work, check whether all union variants are classes
    // and that they have some parent or interface (otherwise they would never
    // have a common base)
    for (const auto *item : as_pipe->items) {
      if (auto as_primitive = item->try_as<TypeHintPrimitive>()) {
        if (as_primitive->ptype != tp_Null) {
          return ClassPtr{};
        }
        continue; // allow cases like `A|B|null`
      }
      const auto *as_instance = item->try_as<TypeHintInstance>();
      if (!as_instance) {
        return ClassPtr{};
      }
      ClassPtr klass = as_instance->resolve();
      if (!klass) {
        return ClassPtr{};
      }
      if (!klass->parent_class && klass->implements.empty()) {
        return ClassPtr{};
      }
    }

    // now we know that there is a chance to find something in common
    // between them, so we can call a more expensive get_common_base_or_interface() method here;
    // the method below is somewhat conservative, but it's good enough in the usual cases
    ClassPtr result;
    ClassPtr first = as_pipe->items[0]->try_as<TypeHintInstance>()->resolve();
    for (int i = 1; i < as_pipe->items.size(); ++i) {
      if (as_pipe->items[i]->try_as<TypeHintPrimitive>()) {
        continue; // we know that it's tp_Null
      }
      ClassPtr other = as_pipe->items[i]->try_as<TypeHintInstance>()->resolve();
      const auto common_bases = first->get_common_base_or_interface(other);
      if (common_bases.size() != 1) {
        return ClassPtr{};
      }
      if (!result || common_bases[0]->is_parent_of(result)) {
        result = common_bases[0];
      } else if (result == common_bases[0] || result->is_parent_of(common_bases[0])) {
        // OK: still compatible.
      } else {
        return ClassPtr{};
      }
    }
    return result;
  }

  if (const auto *as_instance = a->try_as<TypeHintInstance>()) {
    return as_instance->resolve();
  }
  if (const auto *as_callable = a->try_as<TypeHintCallable>()) {
    return as_callable->is_typed_callable() ? as_callable->get_interface() : ClassPtr{};
  }
  return ClassPtr{};
}


// --------------------------------------------

Assumption calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name);

Assumption assumption_get_for_var(FunctionPtr f, vk::string_view var_name) {
  for (const auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      return name_and_a.second;
    }
  }

  return Assumption::undefined();
}

const TypeHint *assumption_merge(const TypeHint *dst, const TypeHint *rhs) {
  if (dst == rhs) {
    return dst;
  }
  if (dst == nullptr || dst->try_as<TypeHintPrimitive>() != nullptr) {
    return rhs;
  }
  if (rhs == nullptr || rhs->try_as<TypeHintPrimitive>() != nullptr) {
    return dst;
  }

  auto merge_classes_lca = [](ClassPtr dst_class, ClassPtr rhs_class) -> ClassPtr {
    if (dst_class->is_parent_of(rhs_class)) {
      return dst_class;
    }
    auto common_bases = rhs_class->get_common_base_or_interface(dst_class);
    return common_bases.size() == 1 ? common_bases[0] : ClassPtr{};
  };

  ClassPtr dst_as_instance = Assumption::extract_instance_from_type_hint(dst);
  ClassPtr rhs_as_instance = Assumption::extract_instance_from_type_hint(rhs);
  if (dst_as_instance && rhs_as_instance) {
    ClassPtr lca_class = merge_classes_lca(dst_as_instance, rhs_as_instance);
    if (!lca_class) {
      return nullptr;
    }
    return TypeHintInstance::create(lca_class->name);
  }

  const auto *dst_as_array = dst->try_as<TypeHintArray>();
  const auto *rhs_as_array = rhs->try_as<TypeHintArray>();
  if (dst_as_array && rhs_as_array) {
    const TypeHint *array_merged = assumption_merge(dst_as_array->inner, rhs_as_array->inner);
    if (array_merged == nullptr) {
      return nullptr;
    }
    return TypeHintArray::create(array_merged);
  }

  const auto *dst_as_tuple = dst->try_as<TypeHintTuple>();
  const auto *rhs_as_tuple = rhs->try_as<TypeHintTuple>();
  if (dst_as_tuple && rhs_as_tuple && dst_as_tuple->items.size() == rhs_as_tuple->items.size()) {
    std::vector<const TypeHint *> items;
    for (int i = 0; i < dst_as_tuple->items.size(); ++i) {
      items.emplace_back(assumption_merge(dst_as_tuple->items[i], rhs_as_tuple->items[i]));
      if (items.back() == nullptr) {
        return nullptr;
      }
    }
    return TypeHintTuple::create(std::move(items));
  }

  const auto *dst_as_shape = dst->try_as<TypeHintShape>();
  const auto *rhs_as_shape = rhs->try_as<TypeHintShape>();
  if (dst_as_shape && rhs_as_shape) {
    std::vector<std::pair<std::string, const TypeHint *>> items;
    for (const auto &it : dst_as_shape->items) {
      items.emplace_back(it.first, assumption_merge(it.second, rhs_as_shape->find_at(it.first)));
      if (items.back().second == nullptr) {
        return nullptr;
      }
    }
    return TypeHintShape::create(std::move(items), false);
  }

  return nullptr;
}

void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const Assumption &assumption) {
  if (!assumption.has_instance()) {   // we store only meaningful assumptions, that will help resolve -> access
    return;
  }

  for (auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      Assumption before = name_and_a.second;
      kphp_error(name_and_a.second = Assumption(assumption_merge(before.assum_hint, assumption.assum_hint)),
                 fmt_format("{}()::${} is both {} and {}\n", f->get_human_readable_name(), var_name, before.as_human_readable(), assumption.as_human_readable()));
      return;
    }
  }

  f->assumptions_for_vars.emplace_back(std::string(var_name), assumption);
//  printf("%s() $%s %s\n", f->name.c_str(), std::string(var_name).c_str(), assumption.as_human_readable().c_str());
}

void assumption_add_for_return(FunctionPtr f, const Assumption &assumption) {
  if (!assumption.has_instance()) {   // we store only meaningful assumptions, that will help resolve -> access
    return;
  }

  if (f->assumption_for_return) {
    Assumption before = f->assumption_for_return;
    kphp_error(f->assumption_for_return = Assumption(assumption_merge(before.assum_hint, assumption.assum_hint)),
               fmt_format("{}() returns both {} and {}\n", f->get_human_readable_name(), before.as_human_readable(), assumption.as_human_readable()));
    return;
  }

  f->assumption_for_return = assumption;
//  printf("%s() returns %s\n", f->name.c_str(), assumption.as_human_readable().c_str());
}


/*
 * Deduce which type is assigned to $a in '$a = ...rhs...'
 */
void analyze_set_to_var(FunctionPtr f, vk::string_view var_name, const VertexPtr &rhs, size_t depth) {
  Assumption rhs_assum = infer_class_of_expr(f, rhs, depth + 1);
  assumption_add_for_var(f, var_name, rhs_assum);
}

/*
 * Deduce which type is assigned to $lhs_var in 'list(... $lhs_var ...) = ...rhs...';
 * (rhs may contain shape or tuple)
 */
void analyze_set_to_list_var(FunctionPtr f, vk::string_view var_name, VertexPtr index_key, const VertexPtr &rhs, size_t depth) {
  Assumption rhs_assum = infer_class_of_expr(f, rhs, depth + 1);
  assumption_add_for_var(f, var_name, rhs_assum.get_subkey_by_index(index_key));
}

/*
 * Deduce that $ex is T-typed from `catch (T $ex)`
 */
void analyze_catch_of_var(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_catch> root) {
  kphp_assert(root->exception_class);
  assumption_add_for_var(f, var_name, Assumption(root->exception_class));
}

/*
 * Deduce that $a is an instance of T if $arr is T[] in 'foreach($arr as $a)'
 */
static void analyze_foreach(FunctionPtr f, vk::string_view var_name, VertexAdaptor<op_foreach_param> root, size_t depth) {
  Assumption arr_assum = infer_class_of_expr(f, root->xs(), depth + 1);
  assumption_add_for_var(f, var_name, arr_assum.get_inner_if_array());
}

/*
 * Deduce that $MC is Memcache from 'global $MC'.
 * Variables like $MC, $MC_Local and other are builtins with a known type.
 */
void analyze_global_var(FunctionPtr f, vk::string_view var_name) {
  if (vk::any_of_equal(var_name, "MC", "MC_Local", "MC_Ads", "MC_Log", "PMC", "mc_fast", "MC_Config", "MC_Stats")) {
    assumption_add_for_var(f, var_name, Assumption(G->get_memcache_class()));
  }
}


/*
 * For the '$a->...' expression (where $a is a local variable) we need to deduce type of $a.
 * This function tries to accomplish that.
 */
void calc_assumptions_for_var_internal(FunctionPtr f, vk::string_view var_name, VertexPtr root, size_t depth) {
  switch (root->type()) {
    case op_phpdoc_var: {
      auto phpdoc_var = root.as<op_phpdoc_var>();
      if (phpdoc_var->var()->str_val == var_name) {
        assumption_add_for_var(f, phpdoc_var->var()->str_val, Assumption(phpdoc_var->type_hint));
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
  for (auto i : f->get_params()) {
    auto param = i.as<op_func_param>();
    if (param->type_hint) {
      assumption_add_for_var(f, param->var()->get_string(), Assumption(param->type_hint));
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
        ClassPtr klass = a.try_as_class() ? a.try_as_class() : a.get_inner_if_array() ? a.get_inner_if_array().try_as_class() : ClassPtr{}; // ?: isn't compiled by gcc
        kphp_error_act(klass, fmt_format("try to get type of field({}) of non-instance", field_name), return false);
        a = calc_assumption_for_class_var(klass, field_name);
      }

      if (a.try_as_class() && return_type_arr_of_arg_type) {
        assumption_add_for_return(f, Assumption(TypeHintArray::create(a.assum_hint)));
      } else if (a.get_inner_if_array() && return_type_element_of_arg_type && field_name.empty()) {
        assumption_add_for_return(f, a.get_inner_if_array());
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

  if (!f->phpdoc_str.empty() && f->is_instantiation_of_template_function()) {
    if (parse_kphp_return_doc(f)) {
      return;
    }
  }
  if (f->return_typehint && !f->is_template) {
    const auto *as_callable = f->return_typehint->try_as<TypeHintCallable>();
    if (!as_callable || as_callable->is_typed_callable()) {
      assumption_add_for_return(f, Assumption(f->return_typehint));
      return;
    }
  }

  // traverse function body, find all 'return' there
  // note, that for strong typed code, with everything annotated with @return, it isn't executed: returned earlier
  std::function<void(VertexPtr)> search_for_returns = [&search_for_returns, f](VertexPtr i) {
    if (i->type() == op_return && i.as<op_return>()->has_expr()) {
      VertexPtr expr = i.as<op_return>()->expr();

      if (expr->type() == op_var && expr->get_string() == "this" && f->modifiers.is_instance()) {
        assumption_add_for_return(f, Assumption(f->class_id));  // return this
      } else if (auto call_vertex = expr.try_as<op_func_call>()) {
        if (call_vertex->str_val == ClassData::NAME_OF_CONSTRUCT && !call_vertex->args().empty()) {
          if (auto alloc = call_vertex->args()[0].try_as<op_alloc>()) {
            assumption_add_for_return(f, Assumption(G->get_class(alloc->allocated_class_name)));
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


Assumption calc_assumption_for_argument(FunctionPtr f, vk::string_view var_name) {
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
Assumption calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth) {
  if (f->modifiers.is_instance() && var_name == "this") {
    return Assumption(f->class_id);
  }

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
      return calc_assumption_for_class_var(of_class, var_name.substr(pos + 2));
    }
  }

  // here we store even 'any' assumptions: we tried to calculate, but var_name has no instances
  f->assumptions_for_vars.emplace_back(std::string{var_name}, Assumption::not_instance());
  return Assumption::not_instance();
}

/*
 * A high-level function which deduces the result type of f.
 * The results are cached; init on f is called during the first invocation.
 * Always returns a non-null assumption.
 */
Assumption calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call) {
  if (f->is_extern()) {
    if (f->return_typehint) {
      const auto *return_typehint = f->return_typehint;
      if (const auto *optional_arg = return_typehint->try_as<TypeHintOptional>()) {
        return_typehint = optional_arg->inner;
      }
      if (const auto *as_instance = return_typehint->try_as<TypeHintInstance>()) {
        return Assumption(as_instance->resolve());
      }
      if (const auto *as_arg_ref = return_typehint->try_as<TypeHintArgRefInstance>()) {
        if (auto arg = GenTree::get_call_arg_ref(as_arg_ref->arg_num, call)) {
          if (const auto *class_name = GenTree::get_constexpr_string(arg)) {
            if (auto klass = G->get_class(*class_name)) {
              return Assumption(klass);
            }
          }
        }
      }
      if (const auto *as_ffi = return_typehint->try_as<TypeHintFFIType>()) {
        return Assumption(as_ffi);
      }
      if (const auto *as_ffi_scope = return_typehint->try_as<TypeHintFFIScopeArgRef>()) {
        if (auto arg = GenTree::get_call_arg_ref(as_ffi_scope->arg_num, call)) {
          if (const auto *scope_name = GenTree::get_constexpr_string(arg)) {
            if (auto module_class = G->get_class(FFIRoot::scope_class_name(*scope_name))) {
              return Assumption(module_class);
            }
          }
        }
      }
    }
    return Assumption::not_instance();
  }

  if (f->is_constructor()) {
    return Assumption(f->class_id);
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
      return Assumption::not_instance();
    }
    assumption_busy_wait(f->assumption_return_status);
  }

  return f->assumption_for_return;
}

/*
 * A high-level function which deduces the type of ->nested inside $a instance of the class c.
 * Always returns a non-null assumption.
 */
Assumption calc_assumption_for_class_var(ClassPtr c, vk::string_view var_name) {
  // for lambdas, proxy captured vars to a containing function
  // e.g. function f() { $a = new A;  function() use ($a) { $a->... } ) — $anon->a is $a in f()
  if (c->is_lambda()) {
    return var_name == LambdaClassData::get_parent_this_name()
           ? calc_assumption_for_var(c->construct_function->function_in_which_lambda_was_created, "this")
           : calc_assumption_for_var(c->construct_function->function_in_which_lambda_was_created, var_name);
  }

  // for regular classes, assume $object->a as it's declared with @var or field type hint
  if (const auto *field = c->members.find_by_local_name<ClassMemberInstanceField>(var_name)) {
    return field->type_hint ? Assumption(field->type_hint) : Assumption::not_instance();
  }
  if (const auto *field = c->members.find_by_local_name<ClassMemberStaticField>(var_name)) {
    return field->type_hint ? Assumption(field->type_hint) : Assumption::not_instance();
  }

  return Assumption::undefined();
}


// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————



Assumption infer_from_var(FunctionPtr f,
                          VertexAdaptor<op_var> var,
                          size_t depth) {
  return calc_assumption_for_var(f, var->str_val, depth + 1);
}


Assumption infer_from_call(FunctionPtr f,
                           VertexAdaptor<op_func_call> call,
                           size_t depth) {
  const std::string &fname = call->extra_type == op_ex_func_call_arrow
                             ? resolve_instance_func_name(f, call)
                             : get_full_static_member_name(f, call->str_val);

  const FunctionPtr ptr = G->get_function(fname);
  if (!ptr) {
    kphp_error(0, fmt_format("{}() is undefined, can not infer class", fname));
    return Assumption::not_instance();
  }

  // for builtin functions like array_pop/array_filter/etc with array of instances
  if (ptr->return_typehint && ptr->return_typehint->has_argref_inside()) {
    if (const auto *as_arg_ref = ptr->return_typehint->try_as<TypeHintArgRef>()) {       // array_values ($a ::: array) ::: ^1
      return infer_class_of_expr(f, GenTree::get_call_arg_ref(as_arg_ref->arg_num, call), depth + 1);
    }
    if (const auto *as_sub = ptr->return_typehint->try_as<TypeHintArgSubkeyGet>()) {  // array_shift (&$a ::: array) ::: ^1[*]
      if (const auto *arg_ref = as_sub->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref->arg_num, call), depth + 1);
        if (Assumption inner = expr_a.get_inner_if_array()) {
          return inner;
        }
      }
      return Assumption::not_instance();
    }
    if (const auto *as_array = ptr->return_typehint->try_as<TypeHintArray>()) {  // f (...) ::: array <^2>
      if (const auto *as_arg_ref = as_array->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = infer_class_of_expr(f, GenTree::get_call_arg_ref(as_arg_ref->arg_num, call), depth + 1);
        if (const auto *inst_a = expr_a ? expr_a.assum_hint->try_as<TypeHintCallable>() : nullptr) {
          return Assumption(TypeHintArray::create(TypeHintInstance::create(inst_a->get_interface()->name)));
        }
        return Assumption::not_instance();
      }
    }

  }

  return calc_assumption_for_return(ptr, call);
}

Assumption infer_from_array(FunctionPtr f,
                            VertexAdaptor<op_array> array,
                            size_t depth) {
  if (array->size() == 0) {
    return Assumption::not_instance();
  }

  ClassPtr klass;
  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    ClassPtr as_klass = infer_class_of_expr(f, v, depth + 1).try_as_class();
    if (!as_klass) {
      return Assumption::not_instance();
    }

    if (!klass) {
      klass = as_klass;
    } else if (klass != as_klass) {
      return Assumption::not_instance();
    }
  }

  return Assumption(TypeHintArray::create(TypeHintInstance::create(klass->name)));
}

Assumption infer_from_tuple(FunctionPtr f,
                            VertexAdaptor<op_tuple> tuple,
                            size_t depth) {
  std::vector<const TypeHint *> sub;
  for (auto sub_expr : *tuple) {
    sub.emplace_back(infer_class_of_expr(f, sub_expr, depth + 1).assum_hint);
  }
  return Assumption(TypeHintTuple::create(std::move(sub)));
}

Assumption infer_from_shape(FunctionPtr f,
                            VertexAdaptor<op_shape> shape,
                            size_t depth) {
  std::vector<std::pair<std::string, const TypeHint *>> sub;
  for (auto sub_expr : shape->args()) {
    auto double_arrow = sub_expr.as<op_double_arrow>();
    sub.emplace_back(GenTree::get_actual_value(double_arrow->lhs())->get_string(), infer_class_of_expr(f, double_arrow->rhs(), depth + 1).assum_hint);
  }
  return Assumption(TypeHintShape::create(std::move(sub), false));
}

Assumption infer_from_pair_vertex_merge(FunctionPtr f, Location location, VertexPtr a, VertexPtr b, size_t depth) {
  auto a_assumption = infer_class_of_expr(f, a, depth + 1);
  auto b_assumption = infer_class_of_expr(f, b, depth + 1);
  if (a_assumption.has_instance() && b_assumption.has_instance()) {
    std::swap(*stage::get_location_ptr(), location);
    Assumption before = a_assumption;
    kphp_error(a_assumption = Assumption(assumption_merge(a_assumption.assum_hint, b_assumption.assum_hint)),
               fmt_format("result of operation is both {} and {}\n", before.as_human_readable(), b_assumption.as_human_readable()));
    std::swap(*stage::get_location_ptr(), location);
  }
  return a_assumption.has_instance() ? a_assumption : b_assumption;
}

Assumption infer_from_ternary(FunctionPtr f, VertexAdaptor<op_ternary> ternary, size_t depth) {
  // this code handles '$x = $y ?: $z' operation;
  // it's converted to '$x = bool($tmp_var = $y) ? move($tmp_var) : $z'
  if (auto move_true_expr = ternary->true_expr().try_as<op_move>()) {
    if (auto tmp_var = move_true_expr->expr().try_as<op_var>()) {
      calc_assumptions_for_var_internal(f, tmp_var->get_string(), ternary->cond(), depth + 1);
    }
  }

  return infer_from_pair_vertex_merge(f, ternary->get_location(), ternary->true_expr(), ternary->false_expr(), depth);
}

Assumption infer_from_instance_prop(FunctionPtr f,
                                    VertexAdaptor<op_instance_prop> prop,
                                    size_t depth) {
  ClassPtr lhs_class = infer_class_of_expr(f, prop->instance(), depth + 1).try_as_class();
  if (!lhs_class) {
    return Assumption::not_instance();
  }

  Assumption res_assum;
  do {
    res_assum = calc_assumption_for_class_var(lhs_class, prop->str_val);
    lhs_class = lhs_class->parent_class;
  } while (!res_assum.has_instance() && lhs_class);

  return res_assum;
}

/*
 * Main functions that are called from the outside: returns the assumptions for any root expression inside f.
 * May return an undefined / not instance / meaningful assumption, extern call should rely only on has_instance().
 */
Assumption infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth /*= 0*/) {
  if (depth > 1000) {
    return Assumption::not_instance();
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
    case op_ffi_load_call:
      return infer_from_call(f, root.as<op_ffi_load_call>()->func_call(), depth + 1);
    case op_ffi_addr:
      return infer_class_of_expr(f, root.as<op_ffi_addr>()->expr(), depth + 1);
    case op_ffi_cast:
      return Assumption(root.as<op_ffi_cast>()->php_type);
    case op_ffi_new:
      return Assumption(root.as<op_ffi_new>()->php_type);
    case op_ffi_c2php_conv:
      return Assumption(root.as<op_ffi_c2php_conv>()->php_type);
    case op_ffi_cdata_value_ref:
      return infer_class_of_expr(f, root.as<op_ffi_cdata_value_ref>()->expr(), depth + 1);
    case op_index: {
      auto index = root.as<op_index>();
      if (index->has_key()) {
        return infer_class_of_expr(f, index->array(), depth + 1).get_subkey_by_index(index->key());
      }
      return Assumption::not_instance();
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
      return Assumption::not_instance();
  }
}



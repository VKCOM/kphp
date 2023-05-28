// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

/*
 * 'Assumption' is predicted type information calculated BEFORE type inferring.
 * It's supposed that type inferring would afterward produce the same types.
 * Assumptions are used to bind function calls (to create call graph):
 * - resolve ->arrowCalls() to detect, a method of which class is actually being called
 *   (for `$a->getValue()` we assume $a to be of class A; hence we set call->func_id = A::getValue).
 * - instantiate generics: having `f<T>` called with `f($a)`, reify T=A
 *
 * Assumptions can be calculated for return values and variables.
 * - for returns of functions, they are from @return or by analyzing 'return' statements
 * - for local variables, they are from @var or by analyzing assignments
 * - for parameters, they are from @param
 *
 * Important!
 * For variables, assumptions rely on variable names, which is generally wrong,
 * because variables can be splitted and smartcasted.
 * See: `function f(mixed $id) { $id = (int)$id; echo $id; }` is later converted to `{ $id$v1 = (int)$id; echo $id$v1; }`
 * That's why assumptions for variables ARE NOT SAVED if they don't contain INSTANCES.
 * We deny splitting variables with instances, because it may lead to mismatch inferring and assumptions.
 * The only exception is lambdas, assumptions for parameters are always stored (for better generics user experience).
 * Return values, nevertheless, can contain assumptions for primitives only.
 *
 * In other words, assumptions are our type expectations/intentions for classes, for earlier (apriori) binding.
 * They are checked by tinf later.
 * If they are incorrect (for example, @return A, but actually returns B), a tinf error will occur later on.
 */
#include "compiler/class-assumptions.h"

#include <thread>

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/pipes/instantiate-ffi-operations.h"
#include "compiler/pipes/deduce-implicit-types-and-casts.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/function-pass.h"
#include "compiler/vertex.h"
#include "compiler/vertex-util.h"


static inline const TypeHint *assumption_unwrap_optional(const TypeHint *assum_hint) {
  if (assum_hint == nullptr) {
    return nullptr;
  }
  if (const auto *as_optional = assum_hint->try_as<TypeHintOptional>()) {
    return as_optional->inner;
  }
  return assum_hint;
}

static inline bool assumption_needs_to_be_saved(const TypeHint *assum_hint) {
  return assum_hint && assum_hint->is_typedata_constexpr() && !assum_hint->has_autogeneric_inside();
}


Assumption::Assumption(ClassPtr klass) : assum_hint(klass->type_hint) {
}

std::string Assumption::as_human_readable() const {
  return assum_hint ? assum_hint->as_human_readable() : "undefined";
}

Assumption Assumption::get_inner_if_array() const {
  const TypeHint *type_hint = assumption_unwrap_optional(assum_hint);

  if (type_hint == nullptr) {
    return {};
  }
  if (const auto *as_array = type_hint->try_as<TypeHintArray>()) {
    return Assumption(as_array->inner);
  }
  return {};
}

Assumption Assumption::get_subkey_by_index(VertexPtr index_key) const {
  const TypeHint *type_hint = assumption_unwrap_optional(assum_hint);

  if (type_hint == nullptr) {
    return {};
  }
  if (const auto *as_array = type_hint->try_as<TypeHintArray>()) {
    return Assumption(as_array->inner);
  }
  if (const auto *as_tuple = type_hint->try_as<TypeHintTuple>()) {
    if (auto as_int_index = VertexUtil::get_actual_value(index_key).try_as<op_int_const>()) {
      int int_index = parse_int_from_string(as_int_index);
      if (int_index >= 0 && int_index < as_tuple->items.size()) {
        return Assumption(as_tuple->items[int_index]);
      }
    }
  }
  if (const auto *as_shape = type_hint->try_as<TypeHintShape>()) {
    if (vk::any_of_equal(VertexUtil::get_actual_value(index_key)->type(), op_int_const, op_string)) {
      const auto &string_index = VertexUtil::get_actual_value(index_key)->get_string();
      if (const TypeHint *at_index = as_shape->find_at(string_index)) {
        return Assumption(at_index);
      }
    }
  }
  return {};
}

static ClassPtr extract_instance_from_ffi_scope(const std::string &scope_name, const FFIType *type) {
  if (const auto *builtin = ffi_builtin_type(type->kind)) {
    return G->get_class(builtin->php_class_name);
  }
  if (vk::any_of_equal(type->kind, FFITypeKind::Struct, FFITypeKind::StructDef, FFITypeKind::Union)) {
    return G->get_class(FFIRoot::cdata_class_name(scope_name, type->str));
  }
  if (type->kind == FFITypeKind::Pointer) {
    return extract_instance_from_ffi_scope(scope_name, type->members[0]);
  }
  return {};
}

// we have to resolve $a->..., knowing (by phpdoc or rough inferring) the TypeHint of a
ClassPtr Assumption::extract_instance_from_type_hint(const TypeHint *a) {
  if (a == nullptr) {
    return ClassPtr{};
  }
  // for T|false or T|null, narrow down to T
  if (const auto *as_optional = a->try_as<TypeHintOptional>()) {
    a = as_optional->inner;
  }

  // the simpliest (and most common) case: just A
  if (const auto *as_instance = a->try_as<TypeHintInstance>()) {
    return as_instance->resolve();
  }
  // for A|B, when A and B all are instances and have a common base
  if (const auto *as_pipe = a->try_as<TypeHintPipe>()) {
    kphp_assert(as_pipe->items.size() >= 2);
    // to avoid the redundant work, check whether all union variants are classes
    // and that they have some parent or interface (otherwise they would never have a common base)
    for (const auto *item : as_pipe->items) {
      if (const auto *as_primitive = item->try_as<TypeHintPrimitive>()) {
        if (as_primitive->ptype != tp_Null) {
          return ClassPtr{};
        }
        continue; // allow cases like `A|B|null`
      }
      ClassPtr klass = extract_instance_from_type_hint(item);
      bool has_parents = klass && (klass->parent_class || !klass->implements.empty());
      if (!has_parents) {
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
      ClassPtr other = extract_instance_from_type_hint(as_pipe->items[i]);
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
  // an instantiated callable or a typed callable (which is called with __invoke for example)
  if (const auto *as_callable = a->try_as<TypeHintCallable>()) {
    return as_callable->is_typed_callable() ? as_callable->get_interface() : as_callable->get_lambda_class();
  }
  // 'T::fieldName' — a syntax that's used in @return for generics
  if (const auto *as_field_ref = a->try_as<TypeHintRefToField>()) {
    if (const TypeHint *field_type_hint = as_field_ref->resolve_field_type_hint()) {
      return extract_instance_from_type_hint(field_type_hint);
    }
  }
  // 'T::methodName()' — a syntax that's used in @return for generics
  if (const auto *as_method_ref = a->try_as<TypeHintRefToMethod>()) {
    if (FunctionPtr method = as_method_ref->resolve_method()) {
      return assume_return_of_function(method).try_as_class();
    }
  }
  // ffi_cdata<scope, struct T> — return T inside scope, it's a registered class
  if (const auto *as_ffi = a->try_as<TypeHintFFIType>()) {
    return extract_instance_from_ffi_scope(as_ffi->scope_name, as_ffi->type);
  }
  // ffi_scope<name> — return the corresponding registered class of this scope
  if (const auto *as_ffi_scope = a->try_as<TypeHintFFIScope>()) {
    return G->get_class(FFIRoot::scope_class_name(as_ffi_scope->scope_name));
  }


  return ClassPtr{};
}


// --------------------------------------------

static Assumption calc_assumption_for_class_static_var(ClassPtr c, const std::string &var_name);
static Assumption calc_assumption_for_class_instance_field(ClassPtr c, const std::string &var_name);


static Assumption assumption_merge(const Assumption &dst, const Assumption &rhs) {
  if (dst.assum_hint == rhs.assum_hint) {
    return dst;
  }

  const TypeHint *dst_hint = assumption_unwrap_optional(dst.assum_hint);
  const TypeHint *rhs_hint = assumption_unwrap_optional(rhs.assum_hint);
  if (dst_hint == rhs_hint) {
    return dst.assum_hint->try_as<TypeHintOptional>() ? dst : rhs;
  }

  const auto *dst_as_instance = dst_hint->try_as<TypeHintInstance>();
  const auto *rhs_as_instance = rhs_hint->try_as<TypeHintInstance>();
  if (dst_as_instance && rhs_as_instance && dst_as_instance->resolve()->is_parent_of(rhs_as_instance->resolve())) {
    return dst.assum_hint->try_as<TypeHintOptional>() ? dst : Assumption(dst_as_instance);
  }

  return {};
}

static void __attribute__((noinline)) print_error_assumptions_arent_merged_for_var(FunctionPtr f, const std::string &var_name, Assumption a, Assumption b) {
  ClassPtr a_klass = a.try_as_class();
  ClassPtr b_klass = b.try_as_class();
  std::vector<ClassPtr> common_bases = a_klass ? a_klass->get_common_base_or_interface(b_klass) : std::vector<ClassPtr>{};

  kphp_error_return(!f->find_param_by_name(var_name),
                    fmt_format("${} is was declared as @param {}, and later reassigned to {}\nPlease, use different names for local vars and arguments",
                               var_name, TermStringFormat::paint_green(a.as_human_readable()), TermStringFormat::paint_green(b.as_human_readable())));
  kphp_error_return(common_bases.empty(),
                    fmt_format("${} is assumed to be both {} and {}\nAdd /** @var {} ${} */ above all assignments to ${} to resolve this conflict.\nProbably, you want /** @var \\{} ${} */",
                               var_name, TermStringFormat::paint_green(a.as_human_readable()), TermStringFormat::paint_green(b.as_human_readable()), "{type}", var_name, var_name, common_bases.front()->name, var_name));
  kphp_error_return(0,
                    fmt_format("${} is assumed to be both {} and {}\nAdd /** @var {} ${} */ above all assignments to ${} to resolve this conflict.",
                               var_name, TermStringFormat::paint_green(a.as_human_readable()), TermStringFormat::paint_green(b.as_human_readable()), "{type}", var_name, var_name));
}

static void __attribute__((noinline)) print_error_assumptions_arent_merged_for_return(FunctionPtr f, Assumption a, Assumption b) {
  kphp_error_return(0,
                    fmt_format("{}() is assumed to return both {} and {}\nAdd /** @return {} */ above a function to resolve this conflict.",
                               f->as_human_readable(), TermStringFormat::paint_green(a.as_human_readable()), TermStringFormat::paint_green(b.as_human_readable()), "{type}"));
}


void assumption_add_for_var(FunctionPtr f, const std::string &var_name, const Assumption &assumption, VertexPtr v_location) {
  if (!assumption_needs_to_be_saved(assumption.assum_hint)) {
    return;
  }
  if (!assumption.assum_hint->has_instances_inside() && !assumption.assum_hint->has_callables_inside()) {
    const bool add_even_for_primitives = f->is_lambda() && f->find_param_by_name(var_name);
    if (!add_even_for_primitives) { // see a comment at the top
      return;
    }
  }

  for (auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      if (Assumption merged = assumption_merge(name_and_a.second, assumption)) {
        name_and_a.second = merged;
      } else {
        stage::set_location(v_location->location);
        print_error_assumptions_arent_merged_for_var(f, var_name, name_and_a.second, assumption);
      }
      return;
    }
  }

  f->assumptions_for_vars.emplace_front(var_name, assumption);
//  printf("%s() $%s %s\n", f->name.c_str(), var_name.c_str(), assumption.as_human_readable().c_str());
}

void assumption_add_for_return(FunctionPtr f, const Assumption &assumption, VertexPtr v_location) {
  if (!assumption_needs_to_be_saved(assumption.assum_hint)) {
    return;
  }

  if (f->assumption_for_return) {
    if (Assumption merged = assumption_merge(f->assumption_for_return, assumption)) {
      f->assumption_for_return = merged;
    } else {
      stage::set_location(v_location->location);
      print_error_assumptions_arent_merged_for_return(f, f->assumption_for_return, assumption);
    }
    return;
  }

  f->assumption_for_return = assumption;
//  printf("%s() returns %s\n", f->as_human_readable().c_str(), assumption.as_human_readable().c_str());
}


// this pass is executed on demand to calculate an assumption for $var_name inside a function
// it traverses function body, searching for "$var_name = rhs" and other cases (trivial, must most common type-affecting ones)
// note, that having a function() { $a = new A; $a->method(); $a = new B; } we need to know the assumption in the middle,
// at the vertex stop_at points to $a->method() call, so it looks up only BEFORE stop_at, assuming A at this point
// (an assumption for B is appended afterward from DeduceImplicitTypesAndCastsPass, resulting in an appropriate error)
class CalcAssumptionForVarPass final : public FunctionPassBase {
  VertexPtr stop_at;
  const std::string &var_name;
  bool stopped{false};

public:
  std::string get_description() override {
    return "Calc assumption for var";
  }

  explicit CalcAssumptionForVarPass(const std::string &var_name, VertexPtr stop_at)
    : stop_at(stop_at)
    , var_name(var_name) {}

  void on_start() override {
//    printf("start assumptions for %s() $%s\n", current_function->as_human_readable().c_str(), std::string(var_name).c_str());
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (stopped) {
      return root;
    }

    if (auto as_catch = root.try_as<op_catch>()) {
      if (as_catch->var()->str_val == var_name) {
        analyze_catch_of_var(as_catch);
      }
    }

    return root;
  }

  // on_exit_vertex (not on_enter) is important: like in DeduceImplicitTypesAndCastsPass
  VertexPtr on_exit_vertex(VertexPtr root) override {
    if (root == stop_at) {
      stopped = true;
      return root;
    }
    if (stopped) {
      return root;
    }

    if (auto as_set = root.try_as<op_set>()) {
      if (as_set->lhs()->type() == op_var && as_set->lhs().as<op_var>()->str_val == var_name) {
        analyze_set_to_var(as_set->rhs(), as_set->rhs());
      }
    } else if (auto as_list = root.try_as<op_list>()) {
      for (auto x : as_list->list()) {
        auto kv = x.as<op_list_keyval>();
        if (kv->var()->type() == op_var && kv->var()->get_string() == var_name) {
          analyze_set_to_list_var(kv->key(), as_list->array(), as_list->array());
        }
      }
    } else if (auto as_foreach = root.try_as<op_foreach_param>()) {
      if (as_foreach->x()->type() == op_var && as_foreach->x()->get_string() == var_name) {
        analyze_foreach(as_foreach, as_foreach);
      }
    } else if (auto as_global = root.try_as<op_global>()) {
      if (as_global->args()[0]->type() == op_var && as_global->args()[0]->get_string() == var_name) {
        analyze_global_var();
      }
    }

    return root;
  }

  // assume which type is assigned to $a in '$a = ...rhs...'
  void analyze_set_to_var(VertexPtr rhs, VertexPtr stop_at) {
    Assumption rhs_assum = assume_class_of_expr(current_function, rhs, stop_at);
    assumption_add_for_var(current_function, var_name, rhs_assum, rhs);
  }

  // assume which type is assigned to $lhs_var in 'list(... $lhs_var ...) = ...rhs...'
  // (rhs may contain shape or tuple)
  void analyze_set_to_list_var(VertexPtr index_key, VertexPtr rhs, VertexPtr stop_at) {
    Assumption rhs_assum = assume_class_of_expr(current_function, rhs, stop_at);
    assumption_add_for_var(current_function, var_name, rhs_assum.get_subkey_by_index(index_key), rhs);
  }

  // assume that $a is an instance of T if $arr is T[] in 'foreach($arr as $a)'
  void analyze_foreach(VertexAdaptor<op_foreach_param> root, VertexPtr stop_at) {
    Assumption arr_assum = assume_class_of_expr(current_function, root->xs(), stop_at);
    assumption_add_for_var(current_function, var_name, arr_assum.get_inner_if_array(), root);
  }

  // assume that $MC is Memcache from 'global $MC'
  // variables like $MC, $MC_Local and other are builtins with a known type
  void analyze_global_var() {
    if (vk::any_of_equal(var_name, "MC", "MC_Local", "MC_Ads", "MC_Log", "PMC", "mc_fast", "MC_Config", "MC_Stats")) {
      assumption_add_for_var(current_function, var_name, Assumption(G->get_memcache_class()), current_function->root);
    }
  }

  // assume that $ex is T-typed from `catch (T $ex)`
  void analyze_catch_of_var(VertexAdaptor<op_catch> root) {
    kphp_assert(root->exception_class);
    assumption_add_for_var(current_function, var_name, Assumption(root->exception_class), root);
  }
};

// this pass is executed on demand and searches for 'return' statements inside a function — to calculate a return assumption
// note, that for strong typed code, with everything annotated with @return, this pass usually isn't executed
// (with the exception for lambdas, i.e. $f = fn()=>fn()=>new A and f()()->method(), when we need an assumption and no @return)
class CalcAssumptionForReturnPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Calc assumptions for return";
  }

  void on_start() override {
//    printf("start assumptions for %s() return\n", current_function->as_human_readable().c_str());
  }

  VertexPtr on_enter_vertex(VertexPtr root) override {
    auto as_return = root.try_as<op_return>();
    if (!as_return || !as_return->has_expr()) {
      return root;
    }

    Assumption ret_assum = assume_class_of_expr(current_function, as_return->expr(), root);
    assumption_add_for_return(current_function, ret_assum, root);
    return root;
  }
};

/*
 * A high-level function which deduces the type of $a inside f.
 * The results are cached per var_name; init on f is called during the first invocation.
 */
static Assumption calc_assumption_for_var(FunctionPtr f, const std::string &var_name, VertexPtr stop_at) {
  if (f->modifiers.is_instance() && var_name == "this") {
    return Assumption(f->class_id);
  }

  // assumptions for arguments are filled outside from @param: see DeduceImplicitTypesAndCastsPass
  if (Assumption from_argument = f->get_assumption_for_var(var_name)) {
    return from_argument;
  }

  if (!f->uses_list.empty() && f->outer_function) {
    if (var_name == "this") {
      return Assumption(f->get_this_or_topmost_if_lambda()->class_id);
    }
    for (auto v : f->uses_list) {
      if (v->str_val == var_name) {
        return calc_assumption_for_var(f->outer_function, var_name, {});    // empty stop_at is ok here
      }
    }
  }

  if (f->type == FunctionData::func_main || f->type == FunctionData::func_switch) {
    if (var_name == "MC" || var_name == "PMC") {
      return Assumption(G->get_memcache_class());
    }
  }

  auto pos = var_name.find("$$");
  if (pos != std::string::npos) {   // static class variable that is used inside a function
    if (auto of_class = G->get_class(replace_characters(var_name.substr(0, pos), '$', '\\'))) {
      return calc_assumption_for_class_static_var(of_class, var_name.substr(pos + 2));
    }
  }

  CalcAssumptionForVarPass pass(var_name, stop_at);
  run_function_pass(f, &pass);

  return f->get_assumption_for_var(var_name);
}

static Assumption calc_assumption_for_class_instance_field(ClassPtr c, const std::string &var_name) {
  if (const auto *field = c->get_instance_field(var_name)) {
    return field->type_hint ? Assumption(field->type_hint) : Assumption();
  }

  return {};
}

static Assumption calc_assumption_for_class_static_var(ClassPtr c, const std::string &var_name) {
  if (const auto *field = c->get_static_field(var_name)) {
    return field->type_hint ? Assumption(field->type_hint) : Assumption();
  }

  return {};
}


// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————


namespace {

Assumption infer_from_var(FunctionPtr f, VertexAdaptor<op_var> var, VertexPtr stop_at) {
  return calc_assumption_for_var(f, var->str_val, stop_at);
}

Assumption infer_from_call(FunctionPtr f, VertexAdaptor<op_func_call> call, VertexPtr stop_at) {
  FunctionPtr f_called = call->func_id;
  if (!f_called) {
    return {};
  }

  // at first, we handle cases, when an assumption depends on a call
  // it means, that "bare return value of f()" is meaningless, it makes sense only on a current call

  // built-in functions like array_pop/array_filter/instance_cast/etc
  if (f_called->return_typehint && f_called->return_typehint->has_argref_inside()) {
    const TypeHint *return_typehint = f_called->return_typehint->unwrap_optional();

    if (const auto *as_arg_ref = return_typehint->try_as<TypeHintArgRef>()) {       // array_values ($a ::: array) ::: ^1
      return assume_class_of_expr(f, VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, call), stop_at);
    }
    if (const auto *as_sub = return_typehint->try_as<TypeHintArgSubkeyGet>()) {  // array_shift (&$a ::: array) ::: ^1[*]
      if (const auto *arg_ref = as_sub->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = assume_class_of_expr(f, VertexUtil::get_call_arg_ref(arg_ref->arg_num, call), stop_at);
        if (Assumption inner = expr_a.get_inner_if_array()) {
          return inner;
        }
      }
    }
    if (const auto *as_arg_ref = return_typehint->try_as<TypeHintArgRefInstance>()) {   // f (...) ::: instance<^1>
      if (auto arg = VertexUtil::get_call_arg_ref(as_arg_ref->arg_num, call)) {
        if (const auto *class_name = VertexUtil::get_constexpr_string(arg)) {
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
      if (auto arg = VertexUtil::get_call_arg_ref(as_ffi_scope->arg_num, call)) {
        if (const auto *scope_name = VertexUtil::get_constexpr_string(arg)) {
          if (auto module_class = G->get_class(FFIRoot::scope_class_name(*scope_name))) {
            return Assumption(module_class);
          }
        }
      }
    }

    return {};
  }

  // calling a generic function; this func call has been previously reified
  // we require generic functions to have @return declared (either constexpr or with generic T inside)
  if (f_called->is_generic()) {
    kphp_assert(call->reifiedTs);

    if (f_called->return_typehint) {
      return Assumption(phpdoc_replace_genericTs_with_reified(f_called->return_typehint, call->reifiedTs));
    }
    return {};
  }

  // some ffi functions, like $cdef->new('int') and FFI::cast(), also depend on a call, like generic functions
  // but their @return can't be expressed in syntax of _functions.txt: instead, inferring is hardcoded
  if (f_called->is_extern()) {
    if (f_called->name == "FFI$$new") {
      return Assumption(InstantiateFFIOperationsPass::infer_from_ffi_static_new(call));
    }
    if (f_called->name == "FFI$$cast") {
      return Assumption(InstantiateFFIOperationsPass::infer_from_ffi_static_cast(f, call));
    }
    if (f_called->name == "FFI$Scope$$new" && !call->args().empty()) {
      ClassPtr scope_class = assume_class_of_expr(f, call->args()[0], call).try_as_class();
      return Assumption(InstantiateFFIOperationsPass::infer_from_ffi_scope_new(call, scope_class));
    }
    if (f_called->name == "FFI$Scope$$cast" && !call->args().empty()) {
      ClassPtr scope_class = assume_class_of_expr(f, call->args()[0], call).try_as_class();
      return Assumption(InstantiateFFIOperationsPass::infer_from_ffi_scope_cast(f, call, scope_class));
    }
    if (f_called->name == "ffi_array_get") {
      return Assumption(InstantiateFFIOperationsPass::infer_from_ffi_array_get(f, call));
    }
  }

  // when an assumption doesn't depend on a call, the return assumption can be once calculated and cached
  return assume_return_of_function(f_called);
}

Assumption infer_from_array(FunctionPtr f, VertexAdaptor<op_array> array, VertexPtr stop_at) {
  if (array->empty()) {
    return {};
  }

  Assumption common_t;
  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    Assumption elem_t = assume_class_of_expr(f, v, stop_at);
    if (!elem_t) {
      return {};
    }

    if (!common_t) {
      common_t = elem_t;
    } else if (common_t.assum_hint != elem_t.assum_hint) {
      return {};
    }
  }

  return Assumption(TypeHintArray::create(common_t.assum_hint));
}

Assumption infer_from_tuple(FunctionPtr f, VertexAdaptor<op_tuple> tuple, VertexPtr stop_at) {
  std::vector<const TypeHint *> sub;
  for (auto sub_expr : *tuple) {
    sub.emplace_back(assume_class_of_expr(f, sub_expr, stop_at).assum_hint ?: TypeHintPrimitive::create(tp_any));
  }
  return Assumption(TypeHintTuple::create(std::move(sub)));
}

Assumption infer_from_shape(FunctionPtr f, VertexAdaptor<op_shape> shape, VertexPtr stop_at) {
  std::vector<std::pair<std::string, const TypeHint *>> sub;
  for (auto sub_expr : shape->args()) {
    const std::string &shape_key = VertexUtil::get_actual_value(sub_expr->front())->get_string();
    sub.emplace_back(shape_key, assume_class_of_expr(f, sub_expr->back(), stop_at).assum_hint ?: TypeHintPrimitive::create(tp_any));
  }
  return Assumption(TypeHintShape::create(std::move(sub), false));
}

Assumption infer_from_pair_vertex_merge(FunctionPtr f, VertexPtr a, VertexPtr b, VertexPtr stop_at) {
  Assumption a_assumption = assume_class_of_expr(f, a, stop_at);
  Assumption b_assumption = assume_class_of_expr(f, b, stop_at);

  return a_assumption && b_assumption ? assumption_merge(a_assumption, b_assumption) : a_assumption ?: b_assumption;
}

Assumption infer_from_ternary(FunctionPtr f, VertexAdaptor<op_ternary> ternary, VertexPtr stop_at) {
  // handle a short ternary '$x = $y ?: $z' that was converted to '$x = bool($tmp_var = $y) ? move($tmp_var) : $z'
  if (ternary->true_expr()->type() == op_move && ternary->cond()->type() == op_conv_bool) {
    return infer_from_pair_vertex_merge(f, ternary->cond().as<op_conv_bool>()->expr().as<op_set>()->rhs(), ternary->false_expr(), stop_at);
  }

  return infer_from_pair_vertex_merge(f, ternary->true_expr(), ternary->false_expr(), stop_at);
}

Assumption infer_from_null_coalesce(FunctionPtr f, VertexAdaptor<op_null_coalesce> null_coalesce, VertexPtr stop_at) {
  return infer_from_pair_vertex_merge(f, null_coalesce->lhs(), null_coalesce->rhs(), stop_at);
}

Assumption infer_from_instance_prop(FunctionPtr f __attribute__ ((unused)), VertexAdaptor<op_instance_prop> prop, VertexPtr stop_at __attribute__ ((unused))) {
  return prop->class_id ? calc_assumption_for_class_instance_field(prop->class_id, prop->str_val) : Assumption();
}

Assumption infer_from_op_lambda(FunctionPtr f __attribute__ ((unused)), VertexAdaptor<op_lambda> lambda, VertexPtr stop_at __attribute__ ((unused))) {
  return Assumption(TypeHintCallable::create_ptr_to_function(lambda->func_id));
}

Assumption infer_from_invoke_call(FunctionPtr f, VertexAdaptor<op_invoke_call> invoke_call, VertexPtr stop_at) {
  // we have $cb(...) transformed to op_invoke_call($cb, ...), this is called on $cb()->prop
  Assumption cb_assum = assume_class_of_expr(f, invoke_call->args()[0], stop_at);
  const TypeHint *assum_hint = assumption_unwrap_optional(cb_assum.assum_hint) ?: TypeHintPrimitive::create(tp_any);

  // $cb can be a raw callable got from op_lambda, while deducing types (until generics and lambdas are instantiated)
  if (const auto *as_callable = assum_hint->try_as<TypeHintCallable>()) {
    if (as_callable->is_untyped_callable() && as_callable->f_bound_to) {
      return assume_return_of_function(as_callable->f_bound_to);
    } else if (as_callable->is_typed_callable()) {
      return Assumption(as_callable->return_type);
    }
  }

  // $cb can be a lambda class, after all instantiations, when binding invoke calls
  ClassPtr c_lambda = cb_assum.try_as_class();
  if (c_lambda && c_lambda->is_lambda_class()) {
    return assume_return_of_function(c_lambda->get_instance_method("__invoke")->function);
  }

  return {};
}

} // namespace


// a public function of this module: deduce the result type of f
// for `$a = getSome(), $a->...`  or `getSome()->...`, we need to deduce the result type of getSome()
// that information can be obtained from the @return tag or returns analysis in body
Assumption assume_return_of_function(FunctionPtr f) {
  // for regular @return (unless 'callable', 'T' and similar), use it as an assumption
  // (for strongly typed code, it covers almost all cases, except lambdas)
  if (assumption_needs_to_be_saved(f->return_typehint)) {
    return Assumption(f->return_typehint);
  }

  if (!f->assumption_for_return) {
    // important! to calculate assumptions, all call->func_id must be already bound
    // in other words, calculating assumptions goes along with binding call->func_id, instance_prop->var_id, etc.
    // we execute the pass which actually does all this stuff, see comments in that file
    // the pass has second-call prevention in check_function()
    DeduceImplicitTypesAndCastsPass pass;
    run_function_pass(f, &pass);

    // having executed that pass, analyze body
    // we must manually handle concurrent execution here (if many threads want to know return assumption for f)
    auto expected = FunctionData::AssumptionStatus::done_deduce_pass;
    if (f->assumption_pass_status.compare_exchange_strong(expected, FunctionData::AssumptionStatus::processing_returns_in_body)) {
      f->assumption_processing_thread = std::this_thread::get_id();
      CalcAssumptionForReturnPass ret_pass;
      run_function_pass(f, &ret_pass);
      f->assumption_pass_status = FunctionData::AssumptionStatus::done_returns_in_body;
    } else if (expected == FunctionData::AssumptionStatus::processing_returns_in_body) {
      while (f->assumption_pass_status == FunctionData::AssumptionStatus::processing_returns_in_body && f->assumption_processing_thread != std::this_thread::get_id()) {
        std::this_thread::sleep_for(std::chrono::nanoseconds{100});
      }
    }
  }

  return f->assumption_for_return;
}

// the main function of this module, that is called from the outside
// it calculates the assumption for "root" inside "f", up to the "stop_at" vertex
// (stop_at is usually in the middle of the function, at the exact point, when we need an assumption,
//  and if he have to traverse the function body, we start analyzing from the start and finist at stop_at)
Assumption assume_class_of_expr(FunctionPtr f, VertexPtr root, VertexPtr stop_at) {
  switch (root->type()) {
    case op_var:
      return infer_from_var(f, root.as<op_var>(), stop_at);
    case op_instance_prop:
      return infer_from_instance_prop(f, root.as<op_instance_prop>(), stop_at);
    case op_func_call:
      return infer_from_call(f, root.as<op_func_call>(), stop_at);
    case op_invoke_call:
      return infer_from_invoke_call(f, root.as<op_invoke_call>(), stop_at);
    case op_ffi_load_call:
      return infer_from_call(f, root.as<op_ffi_load_call>()->func_call(), stop_at);
    case op_ffi_addr:
      return assume_class_of_expr(f, root.as<op_ffi_addr>()->expr(), stop_at);
    case op_ffi_cast:
      return Assumption(root.as<op_ffi_cast>()->php_type);
    case op_ffi_new:
      return Assumption(root.as<op_ffi_new>()->php_type);
    case op_ffi_c2php_conv:
      return Assumption(root.as<op_ffi_c2php_conv>()->php_type);
    case op_ffi_cdata_value_ref:
      return assume_class_of_expr(f, root.as<op_ffi_cdata_value_ref>()->expr(), stop_at);
    case op_ffi_array_get:
      return Assumption(root.as<op_ffi_array_get>()->c_elem_type);
    case op_index: {
      auto index = root.as<op_index>();
      if (index->has_key()) {
        return assume_class_of_expr(f, index->array(), stop_at).get_subkey_by_index(index->key());
      }
      return {};
    }
    case op_array:
      return infer_from_array(f, root.as<op_array>(), stop_at);
    case op_tuple:
      return infer_from_tuple(f, root.as<op_tuple>(), stop_at);
    case op_shape:
      return infer_from_shape(f, root.as<op_shape>(), stop_at);
    case op_ternary:
      return infer_from_ternary(f, root.as<op_ternary>(), stop_at);
    case op_null_coalesce:
      return infer_from_null_coalesce(f, root.as<op_null_coalesce>(), stop_at);
    case op_lambda:
      return infer_from_op_lambda(f, root.as<op_lambda>(), stop_at);
    case op_conv_array:
    case op_conv_array_l:
      return assume_class_of_expr(f, root.as<meta_op_unary>()->expr(), stop_at);
    case op_clone:
      return assume_class_of_expr(f, root.as<op_clone>()->expr(), stop_at);
    case op_seq_rval:
      return assume_class_of_expr(f, root.as<op_seq_rval>()->back(), stop_at);
    case op_move:
      return assume_class_of_expr(f, root.as<op_move>()->expr(), stop_at);
    case op_set:
      return assume_class_of_expr(f, root.as<op_set>()->rhs(), stop_at);
    case op_define_val:
      return assume_class_of_expr(f, root.as<op_define_val>()->value(), stop_at);

    // assumptions for primitives aren't saved to $local_vars and don't help to resolve ->arrows,
    // but they are useful for generics reification
    case op_int_const:
      return Assumption(TypeHintPrimitive::create(tp_int));
    case op_float_const:
      return Assumption(TypeHintPrimitive::create(tp_float));
    case op_string:
    case op_conv_string:
      return Assumption(TypeHintPrimitive::create(tp_string));
    case op_false:
    case op_true:
      return Assumption(TypeHintPrimitive::create(tp_bool));

    default:
      return {};
  }
}

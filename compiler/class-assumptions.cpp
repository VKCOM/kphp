// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

/*
 * 'Assumption' is predicted type information that describes which variables belong to which classes.
 * Used to resolve arrow method calls: to detect, a method of which class is actually being called.
 * For $a->getValue() we can infer that getValue() belongs to the A class; hence we resolve it to Classes$A$$getValue().
 *
 * Method resolving happens in DeduceImplicitTypesAndCastsPass: it assigns func_id to ->calls() and ->fields.
 * That happens before control flow graph and registering variables, this is why assumptions rely on variable names.
 *
 * We also perform a simple function code analysis that helps infer some types without explicit phpdoc hints.
 * For example, in "$a = new A; $a->foo()" we see that A constructor is called, therefore the type of $a is A
 * and $a->foo can be resolved to Classes$A$$foo().
 *
 * Caution! Assumptions is NOT type inferring, it's much more rough analysis.
 * They are used ONLY to bind -> invocations and to instantiate template functions.
 * They do not contain information about primitives, they do not operate cfg/smartcasts — only variable names.
 * In other words, assumptions are our type expectations/intentions for classes, they are checked by tinf later.
 * If they are incorrect (for example, @return A but actually returns B), a tinf error will occur later on.
 */
#include "compiler/class-assumptions.h"

#include <thread>

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/pipes/instantiate-ffi-operations.h"
#include "compiler/phpdoc.h"
#include "compiler/type-hint.h"
#include "compiler/function-pass.h"
#include "compiler/vertex.h"


static inline const TypeHint *assumption_unwrap_optional(const TypeHint *assum_hint) {
  if (assum_hint == nullptr) {
    return nullptr;
  }
  if (const auto *as_optional = assum_hint->try_as<TypeHintOptional>()) {
    return as_optional->inner;
  }
  return assum_hint;
}

static inline bool assumption_needs_to_be_saved(const Assumption &assumption) {
  const TypeHint *type_hint = assumption_unwrap_optional(assumption.assum_hint);

  if (type_hint == nullptr) {
    return false;
  }
  if (const auto *as_callable = type_hint->try_as<TypeHintCallable>()) {
    // don't treat plain 'callable' as an assumption, until it's bound to an actual function (a lambda, probably)
    return as_callable->is_typed_callable() || as_callable->f_bound_to;
  }
  return true;
}


Assumption::Assumption(const TypeHint *type_hint) {
  if (type_hint == nullptr) {
    assum_hint = nullptr;
  } else if (!type_hint->has_instances_inside() && !type_hint->has_callables_inside()) {
    assum_hint = nullptr;
  } else {
    assum_hint = type_hint; // note, that tuple(int,A) is saved as is, but getting [0] subkey will return undefined
  }
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
    if (auto as_int_index = GenTree::get_actual_value(index_key).try_as<op_int_const>()) {
      int int_index = parse_int_from_string(as_int_index);
      if (int_index >= 0 && int_index < as_tuple->items.size()) {
        return Assumption(as_tuple->items[int_index]);
      }
    }
  }
  if (const auto *as_shape = type_hint->try_as<TypeHintShape>()) {
    if (vk::any_of_equal(GenTree::get_actual_value(index_key)->type(), op_int_const, op_string)) {
      const auto &string_index = GenTree::get_actual_value(index_key)->get_string();
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
      const auto *as_instance = item->try_as<TypeHintInstance>();
      ClassPtr klass = as_instance ? as_instance->resolve() : ClassPtr{};
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
  // an instantiated callable or a typed callable (which is called with __invoke for example)
  if (const auto *as_callable = a->try_as<TypeHintCallable>()) {
    return as_callable->is_typed_callable() ? as_callable->get_interface() : as_callable->get_lambda_class();
  }
  // 'A::fieldName' — a syntax that's used in @kphp-return
  if (const auto *as_field_ref = a->try_as<TypeHintFieldRef>()) {
    if (const auto *field = as_field_ref->resolve_field()) {
      return extract_instance_from_type_hint(field->type_hint);
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
static Assumption calc_assumption_for_return(FunctionPtr f);


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

static void __attribute__((noinline)) print_error_assumptions_arent_merged_for_var(const std::string &var_name, Assumption a, Assumption b) {
  ClassPtr a_klass = a.try_as_class();
  ClassPtr b_klass = b.try_as_class();
  std::vector<ClassPtr> common_bases = a_klass ? a_klass->get_common_base_or_interface(b_klass) : std::vector<ClassPtr>{};

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
  if (!assumption_needs_to_be_saved(assumption)) {   // we store only meaningful assumptions, that will help resolve -> access
    return;
  }

  for (auto &name_and_a : f->assumptions_for_vars) {
    if (name_and_a.first == var_name) {
      if (Assumption merged = assumption_merge(name_and_a.second, assumption)) {
        name_and_a.second = merged;
      } else {
        stage::set_location(v_location->location);
        print_error_assumptions_arent_merged_for_var(var_name, name_and_a.second, assumption);
      }
      return;
    }
  }

  f->assumptions_for_vars.emplace_front(var_name, assumption);
//  printf("%s() $%s %s\n", f->name.c_str(), var_name.c_str(), assumption.as_human_readable().c_str());
}

void assumption_add_for_return(FunctionPtr f, const Assumption &assumption, VertexPtr v_location) {
  if (!assumption_needs_to_be_saved(assumption)) {   // we store only meaningful assumptions, that will help resolve -> access
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
    VertexPtr expr = as_return->expr();

    if (expr->type() == op_var && expr->get_string() == "this" && current_function->modifiers.is_instance()) {
      assumption_add_for_return(current_function, Assumption(current_function->class_id), root);  // return this
    } else if (auto call = expr.try_as<op_func_call>()) {
      if (call->extra_type == op_ex_constructor_call) {
        assumption_add_for_return(current_function, Assumption(call->args()[0].as<op_alloc>()->allocated_class), root);
      } else if (call->func_id) {
        assumption_add_for_return(current_function, calc_assumption_for_return(call->func_id), root);
      }
    } else if (auto call_ex = expr.try_as<op_exception_constructor_call>()) {
      assumption_add_for_return(current_function, Assumption(call_ex->constructor_call()->args()[0].as<op_alloc>()->allocated_class), root);
    } else if (auto lambda_vertex = expr.try_as<op_lambda>()) {
      assumption_add_for_return(current_function, assume_class_of_expr(current_function, lambda_vertex, root), root);
    }

    return root;
  }
};

// this is a hack preventing race condition and should be removed somewhen
// (though it does not prevent it fully, but it happens noticeable more rarely)
// see comment in MR for detailed description
void run_CalcAssumptionForVarPass_safe(FunctionPtr function, CalcAssumptionForVarPass *pass) {
  Location location_backup_raw = Location{*stage::get_location_ptr()};
  pass->setup(function);
  pass->on_start();

  std::function<void(VertexPtr)> recurse = [&recurse, pass](VertexPtr vertex) {
    stage::set_location(vertex->get_location());

    pass->on_enter_vertex(vertex);
    if (!pass->user_recursion(vertex)) {
      for (VertexPtr i : *vertex) {
        recurse(i);
      }
    }
    pass->on_exit_vertex(vertex);
  };

  recurse(function->root);
  pass->on_finish();
  *stage::get_location_ptr() = std::move(location_backup_raw);
}

/*
 * For the '$a = getSome(), $a->... , or getSome()->...' we need to deduce the result type of getSome().
 * That information can be obtained from the @return tag or trivial returns analysis inside that function.
 * Called exactly once for every function.
 */
void init_assumptions_for_return(FunctionPtr f) {
  kphp_assert (f->assumption_return_status == AssumptionStatus::processing);
//  printf("[%d] init_assumptions_for_return of %s\n", get_thread_id(), f->name.c_str());

  if (f->return_typehint) {
    bool is_meaningful =
      f->return_typehint->is_typedata_constexpr() &&
      (!f->return_typehint->try_as<TypeHintCallable>() || f->return_typehint->try_as<TypeHintCallable>()->is_typed_callable());
    if (is_meaningful) {
      assumption_add_for_return(f, Assumption(f->return_typehint), f->root);
      return;
    }
  }

  // traverse function body, find all 'return' there
  CalcAssumptionForReturnPass pass;
  run_function_pass(f, &pass);
}

/*
 * A high-level function which deduces the type of $a inside f.
 * The results are cached per var_name; init on f is called during the first invocation.
 */
static Assumption calc_assumption_for_var(FunctionPtr f, const std::string &var_name, VertexPtr stop_at) {
  if (f->modifiers.is_instance() && var_name == "this") {
    return Assumption(f->class_id);
  }

  // assumptions for arguments are filled outside, like assumptions from phpdocs: see DeduceImplicitTypesAndCastsPass
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
      assumption_add_for_var(f, var_name, Assumption(G->get_memcache_class()), f->root);
    }
  }

  auto pos = var_name.find("$$");
  if (pos != std::string::npos) {   // static class variable that is used inside a function
    if (auto of_class = G->get_class(replace_characters(var_name.substr(0, pos), '$', '\\'))) {
      return calc_assumption_for_class_static_var(of_class, var_name.substr(pos + 2));
    }
  }

  CalcAssumptionForVarPass pass(var_name, stop_at);
  run_CalcAssumptionForVarPass_safe(f, &pass);

  return f->get_assumption_for_var(var_name);
}

/*
 * A high-level function which deduces the result type of f.
 * The results are cached; init on f is called during the first invocation.
 */
static Assumption calc_assumption_for_return(FunctionPtr f) {
  // these cases depend on a call, they are handled in infer_from_call, but if it was bypassed
  if (f->is_template() || (f->return_typehint && !f->return_typehint->is_typedata_constexpr())) {
    return {};
  }

  if (f->is_constructor()) {
    return Assumption(f->class_id);
  }

  auto expected = AssumptionStatus::unknown;
  if (f->assumption_return_status.compare_exchange_strong(expected, AssumptionStatus::processing)) {
    kphp_assert(f->assumption_return_processing_thread == std::thread::id{});
    f->assumption_return_processing_thread = std::this_thread::get_id();
    init_assumptions_for_return(f);
    f->assumption_return_status = AssumptionStatus::initialized;
  } else if (expected == AssumptionStatus::processing) {
    if (f->assumption_return_processing_thread == std::this_thread::get_id()) {
      return {};
    }
    while (f->assumption_return_status != AssumptionStatus::initialized) {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }
  }

  return f->assumption_for_return;
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
      return assume_class_of_expr(f, GenTree::get_call_arg_ref(as_arg_ref->arg_num, call), stop_at);
    }
    if (const auto *as_sub = return_typehint->try_as<TypeHintArgSubkeyGet>()) {  // array_shift (&$a ::: array) ::: ^1[*]
      if (const auto *arg_ref = as_sub->inner->try_as<TypeHintArgRef>()) {
        auto expr_a = assume_class_of_expr(f, GenTree::get_call_arg_ref(arg_ref->arg_num, call), stop_at);
        if (Assumption inner = expr_a.get_inner_if_array()) {
          return inner;
        }
      }
    }
    if (const auto *as_arg_ref = return_typehint->try_as<TypeHintArgRefInstance>()) {   // f (...) ::: instance<^1>
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

    return {};
  }

  // calling a template function; this func call has been previously instantiated
  // then we take a regular @return (a constexpr one) or @kphp-return with genericsT inside (it's also stored in return_typehint)
  if (f_called->is_template()) {
    kphp_assert(call->instantiation_list);

    if (f_called->return_typehint) {
      return Assumption(phpdoc_replace_genericsT_with_instantiation(f_called->return_typehint, call->instantiation_list));
    }
    return {};
  }

  // some ffi functions, like $cdef->new('int') and FFI::cast(), also depend on a call, like template functions
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
  }

  // when an assumption doesn't depend on a call, the return assumption can be once calculated and cached
  return calc_assumption_for_return(f_called);
}

Assumption infer_from_array(FunctionPtr f, VertexAdaptor<op_array> array, VertexPtr stop_at) {
  if (array->empty()) {
    return {};
  }

  ClassPtr klass;
  for (auto v : *array) {
    if (auto double_arrow = v.try_as<op_double_arrow>()) {
      v = double_arrow->value();
    }
    ClassPtr as_klass = assume_class_of_expr(f, v, stop_at).try_as_class();
    if (!as_klass) {
      return {};
    }

    if (!klass) {
      klass = as_klass;
    } else if (klass != as_klass) {
      return {};
    }
  }

  return Assumption(TypeHintArray::create(klass->type_hint));
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
    const std::string &shape_key = GenTree::get_actual_value(sub_expr->front())->get_string();
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

  // $cb can be a raw callable got from op_lambda, while deducing types (until templates and lambdas are instantiated)
  if (const auto *as_callable = assum_hint->try_as<TypeHintCallable>()) {
    if (as_callable->is_untyped_callable() && as_callable->f_bound_to) {
      return calc_assumption_for_return(as_callable->f_bound_to);
    } else if (as_callable->is_typed_callable()) {
      return Assumption(as_callable->return_type);
    }
  }

  // $cb can be a lambda class, after all instantiations, when binding invoke calls
  ClassPtr c_lambda = cb_assum.try_as_class();
  if (c_lambda && c_lambda->is_lambda_class()) {
    return calc_assumption_for_return(c_lambda->get_instance_method("__invoke")->function);
  }

  return {};
}

} // namespace


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
    case op_exception_constructor_call:
      return infer_from_call(f, root.as<op_exception_constructor_call>()->constructor_call(), stop_at);
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
    default:
      return {};
  }
}
